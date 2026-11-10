#include "TiledSDLRenderer.hpp"

#include <SDL3/SDL.h>

#include "../GDY/GDYFactory.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>

#include <spdlog/spdlog.h>

namespace griddly {

TiledSDLRenderer::TiledSDLRenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                                   const std::vector<std::shared_ptr<Grid>>& grids,
                                   TiledSDLRendererConfig config)
    : gdyFactory_(std::move(gdyFactory)), config_(config) {
  if (numEnvs == 0) throw std::invalid_argument("TiledSDLRenderer requires >= 1 env");
  if (grids.size() != numEnvs) throw std::invalid_argument("grids.size() must equal numEnvs");
  numEnvs_ = numEnvs;

  // one SDL observer per env, using the GDY's named observer config
  observers_.reserve(numEnvs_);
  for (uint32_t i = 0; i < numEnvs_; i++) {
    auto obs = std::dynamic_pointer_cast<SDLObserver>(gdyFactory_->createObserver(grids[i], config_.observerName, 1, 0));
    if (obs == nullptr) throw std::invalid_argument("TiledSDLRenderer requires an SDL observer (Sprite2D, Block2D or Isometric)");
    obs->init({});
    obs->reset();
    observers_.push_back(obs);
  }

  // observers have their shape after reset(); every env must render at the same size
  const auto firstShape = observers_[0]->getShape();
  for (const auto& o : observers_) {
    if (o->getShape() != firstShape) throw std::invalid_argument("TiledSDLRenderer requires all envs to have the same observation size");
  }
  computeLayout(numEnvs_, config_, firstShape[1], firstShape[2]);

  // gap colour everywhere, so unused layout slots match the GPU and CUDA paths
  pixelBuffer_.resize((size_t)pixelWidth_ * pixelHeight_ * 4);
  std::fill_n(reinterpret_cast<uint32_t*>(pixelBuffer_.data()), (size_t)pixelWidth_ * pixelHeight_, kTiledGapColor);

  // Composite target: try GPU drivers in turn, fall back to pure-CPU compositing.
  // EGL (opengles2) first: works headless on datacenter GPUs via EGL surfaceless,
  // more portable there than Vulkan (which needs VK_EXT_headless_surface).
  // GRIDDLY_SDL_DRIVER forces a specific driver; GRIDDLY_SDL_FORCE_CPU skips GPU.
  std::string driver;
  bool videoUp = false;
  std::vector<std::string> gpuDrivers;
  if (std::getenv("GRIDDLY_SDL_FORCE_CPU") == nullptr) {
    // Headless: the SDL_HINT_VIDEO_DRIVER hint (set BEFORE our SDL_Init) makes
    // SDL pick the offscreen video driver, which registers all 4 render
    // drivers incl. opengles2 and allows hidden OPENGL windows on the GPU.
    if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr &&
        std::getenv("SDL_VIDEODRIVER") == nullptr) {
      SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    }
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
      // Full SDL_Init (not SDL_InitSubSystem): with the video-driver hint set
      // above it registers all render drivers incl. opengles2 on the GPU.
      if (SDL_Init(SDL_INIT_VIDEO)) {
        videoUp = true;
      } else {
        spdlog::info("TiledSDLRenderer: video subsystem unavailable ({}), probing render drivers headless", SDL_GetError());
      }
    }
    const char* forced = std::getenv("GRIDDLY_SDL_DRIVER");
    if (forced != nullptr) {
      gpuDrivers.emplace_back(forced);
    } else {
      int driverCount = SDL_GetNumRenderDrivers();
      for (int i = 0; i < driverCount; i++) {
        const char* name = SDL_GetRenderDriver(i);
        if (name == nullptr || SDL_strcasecmp(name, "software") == 0) continue;
        // prefer EGL-based driver first
        if (SDL_strcasecmp(name, "opengles2") == 0) gpuDrivers.insert(gpuDrivers.begin(), name);
        else gpuDrivers.emplace_back(name);
      }
    }
  }
  for (const auto& drv : gpuDrivers) {
    window_ = SDL_CreateWindow("Griddly Tiled", (int)pixelWidth_, (int)pixelHeight_, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    if (window_ == nullptr) {
      spdlog::info("TiledSDLRenderer: hidden window failed ({}), trying next driver", SDL_GetError());
      continue;
    }
    renderer_ = SDL_CreateRenderer(window_, drv.c_str());
    if (renderer_ == nullptr) {
      spdlog::info("TiledSDLRenderer: driver '{}' unusable on window ({}), trying next", drv, SDL_GetError());
      SDL_DestroyWindow(window_);
      window_ = nullptr;
      continue;
    }
    usingGPU_ = true;
    driver = drv;
    targetTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                        (int)pixelWidth_, (int)pixelHeight_);
    if (targetTexture_ == nullptr) throw std::runtime_error(fmt::format("TiledSDLRenderer: failed to create {}x{} target: {}", pixelWidth_, pixelHeight_, SDL_GetError()));
    SDL_SetTextureBlendMode(targetTexture_, SDL_BLENDMODE_NONE);
    // gaps and unused layout slots are never drawn over: fill once
    SDL_SetRenderTarget(renderer_, targetTexture_);
    SDL_SetRenderDrawColor(renderer_, kTiledGapRGBA[0], kTiledGapRGBA[1], kTiledGapRGBA[2], kTiledGapRGBA[3]);
    SDL_RenderClear(renderer_);
    if (drv == "opengles2" || drv == "opengl") {
      glReadPixels_ = reinterpret_cast<GLReadPixelsFn>(SDL_GL_GetProcAddress("glReadPixels"));
      glFinish_ = reinterpret_cast<GLFinishFn>(SDL_GL_GetProcAddress("glFinish"));
    }
    spdlog::info("TiledSDLRenderer: compositing with GPU driver '{}' ({}x{} envs, frame {}x{})",
                 drv, cols_, rows_, pixelWidth_, pixelHeight_);
    break;
  }
  if (renderer_ == nullptr) {
    // Pure CPU: no composite renderer, the env observers' software renderers
    // draw straight into pixelBuffer_.
    if (videoUp) SDL_Quit();
    videoUp = false;
    spdlog::info("TiledSDLRenderer: rendering on CPU, each env draws straight into its rect of the composite");
  }

  for (uint32_t i = 0; i < numEnvs_; i++) {
    if (usingGPU_) {
      observers_[i]->setSharedRenderTarget(renderer_, targetTexture_, (float)envOffsetX(i), (float)envOffsetY(i), &sharedSprites_);
    } else {
      uint8_t* origin = pixelBuffer_.data() + ((size_t)envOffsetY(i) * pixelWidth_ + envOffsetX(i)) * 4;
      observers_[i]->setExternalPixelTarget(origin, (int)(pixelWidth_ * 4));
    }
  }
}

TiledSDLRenderer::~TiledSDLRenderer() {
  for (auto& o : observers_) o->release();
  for (auto& sprite : sharedSprites_) SDL_DestroyTexture(sprite.second);
  if (baseTexture_ != nullptr) SDL_DestroyTexture(baseTexture_);
  if (targetTexture_ != nullptr) SDL_DestroyTexture(targetTexture_);
  if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
  if (window_ != nullptr) SDL_DestroyWindow(window_);
  if (surface_ != nullptr) SDL_DestroySurface(surface_);
}

bool TiledSDLRenderer::isUsingGPU() const { return usingGPU_; }

void TiledSDLRenderer::readbackComposite() {
  if (glReadPixels_ != nullptr) {
    // GL drivers: read the target FBO straight into pixelBuffer_. SDL's
    // SDL_RenderReadPixels allocates a new frame-sized surface every call.
    SDL_FlushRenderer(renderer_);
    glReadPixels_(0, 0, (int)pixelWidth_, (int)pixelHeight_, 0x1908 /*GL_RGBA*/, 0x1401 /*GL_UNSIGNED_BYTE*/, pixelBuffer_.data());
    return;
  }
  SDL_Surface* shot = SDL_RenderReadPixels(renderer_, nullptr);
  if (shot == nullptr) return;
  SDL_Surface* conv = shot->format == SDL_PIXELFORMAT_RGBA32 ? shot : SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
  if (conv != nullptr) {
    for (int y = 0; y < conv->h; y++)
      SDL_memcpy(pixelBuffer_.data() + (size_t)y * pixelWidth_ * 4,
                 (const uint8_t*)conv->pixels + (size_t)y * conv->pitch, (size_t)pixelWidth_ * 4);
    if (conv != shot) SDL_DestroySurface(conv);
  }
  SDL_DestroySurface(shot);
}

void TiledSDLRenderer::buildBaseLayer() {
  // Every env shares the observer config and view size, so env 0's clear +
  // background tile is everyone's base layer. Draw it into env 0's rect, then
  // keep a copy.
  baseLayerBuilt_ = true;
  observers_[0]->drawBaseLayerOnly();
  const SDL_FRect rect{(float)envOffsetX(0), (float)envOffsetY(0), (float)envPixelWidth_, (float)envPixelHeight_};
  if (usingGPU_) {
    baseTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, (int)envPixelWidth_, (int)envPixelHeight_);
    if (baseTexture_ == nullptr) return;  // keep clear + background per env
    SDL_SetTextureBlendMode(baseTexture_, SDL_BLENDMODE_NONE);
    SDL_SetTextureScaleMode(baseTexture_, SDL_SCALEMODE_NEAREST);
    SDL_SetRenderTarget(renderer_, baseTexture_);
    SDL_RenderTexture(renderer_, targetTexture_, &rect, nullptr);
    SDL_SetRenderTarget(renderer_, targetTexture_);
    for (auto& o : observers_) o->setBaseLayer(nullptr, baseTexture_);
  } else {
    const size_t rowBytes = (size_t)envPixelWidth_ * 4;
    basePixels_.resize(rowBytes * envPixelHeight_);
    const uint8_t* src = pixelBuffer_.data() + ((size_t)envOffsetY(0) * pixelWidth_ + envOffsetX(0)) * 4;
    for (uint32_t y = 0; y < envPixelHeight_; y++) std::memcpy(basePixels_.data() + y * rowBytes, src + (size_t)y * pixelWidth_ * 4, rowBytes);
    for (auto& o : observers_) o->setBaseLayer(basePixels_.data(), nullptr);
  }
}

bool TiledSDLRenderer::isBatchable(uint32_t i) const {
  const auto& o = *observers_[i];
  if (o.getObserverType() == ObserverType::ISOMETRIC) return false;  // sprites overlap by design
  const auto tile = o.getTileSize();
  const float tw = static_cast<float>(tile.x), th = static_cast<float>(tile.y);
  for (const auto& item : o.preparedItems()) {
    if (item.texture == nullptr) continue;
    if (item.tile) return false;
    const SDL_FRect& d = item.dst;
    if (d.x < 0 || d.y < 0 || d.x + d.w > envPixelWidth_ || d.y + d.h > envPixelHeight_) return false;
    // inside a single tile cell
    const float cx = std::floor(d.x / tw) * tw, cy = std::floor(d.y / th) * th;
    if (d.x + d.w > cx + tw || d.y + d.h > cy + th) return false;
    if (item.angle != 0.0) {
      const double quarters = item.angle / 90.0;
      if (quarters != std::round(quarters) || d.w != d.h) return false;
    }
  }
  return true;
}

void TiledSDLRenderer::drawBatched(const std::vector<uint32_t>& envs) {
  // base layer for every env in one call (blend NONE overwrites the tile)
  batchVertices_.clear();
  batchIndices_.clear();
  auto pushQuad = [&](const SDL_FRect& d, const SDL_FColor& color, const float (&u)[4], const float (&v)[4]) {
    const int base = static_cast<int>(batchVertices_.size());
    const float xs[4] = {d.x, d.x + d.w, d.x + d.w, d.x};
    const float ys[4] = {d.y, d.y, d.y + d.h, d.y + d.h};
    for (int k = 0; k < 4; k++) batchVertices_.push_back(SDL_Vertex{{xs[k], ys[k]}, color, {u[k], v[k]}});
    for (int k : {0, 1, 2, 0, 2, 3}) batchIndices_.push_back(base + k);
  };
  const SDL_FColor white{1.0f, 1.0f, 1.0f, 1.0f};
  const float fullU[4] = {0, 1, 1, 0}, fullV[4] = {0, 0, 1, 1};
  for (uint32_t e : envs) {
    pushQuad(SDL_FRect{(float)envOffsetX(e), (float)envOffsetY(e), (float)envPixelWidth_, (float)envPixelHeight_}, white, fullU, fullV);
  }
  SDL_RenderGeometry(renderer_, baseTexture_, batchVertices_.data(), (int)batchVertices_.size(), batchIndices_.data(), (int)batchIndices_.size());

  // every item, ordered by z then texture: within an env items of equal z
  // are in different cells (one object per z per cell) so they never overlap
  batchQuads_.clear();
  for (uint32_t e : envs) {
    const auto& items = observers_[e]->preparedItems();
    for (uint32_t k = 0; k < items.size(); k++) {
      if (items[k].texture != nullptr) batchQuads_.push_back(BatchQuad{items[k].z, items[k].texture, e, k});
    }
  }
  std::stable_sort(batchQuads_.begin(), batchQuads_.end(), [](const BatchQuad& a, const BatchQuad& b) {
    if (a.z != b.z) return a.z < b.z;
    return a.texture < b.texture;
  });

  size_t runStart = 0;
  while (runStart < batchQuads_.size()) {
    SDL_Texture* texture = batchQuads_[runStart].texture;
    float tw = 0, th = 0;
    SDL_GetTextureSize(texture, &tw, &th);
    batchVertices_.clear();
    batchIndices_.clear();
    size_t k = runStart;
    for (; k < batchQuads_.size() && batchQuads_[k].texture == texture; k++) {
      const auto& q = batchQuads_[k];
      const auto& item = observers_[q.env]->preparedItems()[q.item];
      SDL_FRect d = item.dst;
      d.x += (float)envOffsetX(q.env);
      d.y += (float)envOffsetY(q.env);
      // texture corners TL, TR, BR, BL; a clockwise quarter turn moves each
      // destination corner's texel one corner back
      const float u0 = item.src.x / tw, v0 = item.src.y / th, u1 = (item.src.x + item.src.w) / tw, v1 = (item.src.y + item.src.h) / th;
      const float cu[4] = {u0, u1, u1, u0}, cv[4] = {v0, v0, v1, v1};
      const int quarters = ((static_cast<int>(std::lround(item.angle / 90.0)) % 4) + 4) % 4;
      float u[4], v[4];
      for (int c = 0; c < 4; c++) {
        u[c] = cu[(c - quarters + 4) % 4];
        v[c] = cv[(c - quarters + 4) % 4];
      }
      const SDL_FColor color{item.r / 255.0f, item.g / 255.0f, item.b / 255.0f, item.a / 255.0f};
      pushQuad(d, color, u, v);
    }
    SDL_RenderGeometry(renderer_, texture, batchVertices_.data(), (int)batchVertices_.size(), batchIndices_.data(), (int)batchIndices_.size());
    runStart = k;
  }
  for (uint32_t e : envs) observers_[e]->discardPreparedFrame();
}

const uint8_t* TiledSDLRenderer::render(bool parallel, uint32_t numThreads) {
  const uint32_t threads = parallel ? numThreads : 1;
  if (!baseLayerBuilt_ && std::getenv("GRIDDLY_SDL_NO_BASE_LAYER") == nullptr) buildBaseLayer();
  if (usingGPU_) {
    // One GL context belongs to one thread: first-frame setup and all draw
    // calls happen here. Building render items is CPU work and runs on the pool.
    for (auto& o : observers_) o->ensureReady();
    parallelFor(numEnvs_, threads, 8, [&](uint32_t i) { observers_[i]->prepareFrame(); });
    batchEnvs_.clear();
    fallbackEnvs_.clear();
    const bool batching = baseTexture_ != nullptr && std::getenv("GRIDDLY_SDL_NO_BATCH") == nullptr;
    for (uint32_t i = 0; i < numEnvs_; i++) (batching && isBatchable(i) ? batchEnvs_ : fallbackEnvs_).push_back(i);
    // envs cover disjoint rects of the target, so their relative order is free
    if (!batchEnvs_.empty()) drawBatched(batchEnvs_);
    for (uint32_t i : fallbackEnvs_) observers_[i]->drawPreparedFrame();
    SDL_SetRenderViewport(renderer_, nullptr);

    // single readback of the whole composite. Without it the frame stays on
    // the GPU, but it must still be finished: SDL only queues draw calls, so
    // without a flush nothing would execute and timings would be meaningless.
    if (std::getenv("GRIDDLY_SDL_GPU_NO_READBACK") == nullptr) {
      readbackComposite();
    } else {
      SDL_FlushRenderer(renderer_);
      if (glFinish_ != nullptr) glFinish_();
    }
    if (window_ != nullptr && !(SDL_GetWindowFlags(window_) & SDL_WINDOW_HIDDEN)) {
      SDL_SetRenderTarget(renderer_, nullptr);
      SDL_RenderTexture(renderer_, targetTexture_, nullptr, nullptr);
      SDL_RenderPresent(renderer_);
      SDL_SetRenderTarget(renderer_, targetTexture_);
    }
    return pixelBuffer_.data();
  }

  // CPU path: every observer owns a software renderer drawing straight into
  // its rect of pixelBuffer_, so envs render fully in parallel with no composite copy
  parallelFor(numEnvs_, threads, 1, [&](uint32_t i) { observers_[i]->update(); });
  return pixelBuffer_.data();
}

}  // namespace griddly
