#include "TiledSDLRenderer.hpp"

#include <SDL3/SDL.h>

#include "../GDY/GDYFactory.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <thread>

#include <spdlog/spdlog.h>

namespace griddly {

TiledSDLRenderer::TiledSDLRenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                                   const std::vector<std::shared_ptr<Grid>>& grids,
                                   TiledSDLRendererConfig config)
    : gdyFactory_(std::move(gdyFactory)), config_(config), numEnvs_(numEnvs) {
  if (numEnvs_ == 0) throw std::invalid_argument("TiledSDLRenderer requires >= 1 env");
  if (grids.size() != numEnvs_) throw std::invalid_argument("grids.size() must equal numEnvs");

  // layout: cols x rows, auto = near-square
  cols_ = config_.cols > 0 ? config_.cols : (uint32_t)std::ceil(std::sqrt((double)numEnvs_));
  rows_ = config_.rows > 0 ? config_.rows : (numEnvs_ + cols_ - 1) / cols_;
  gap_ = config_.gap;

  // one SpriteObserver per env, using the GDY's named observer config
  observers_.reserve(numEnvs_);
  for (uint32_t i = 0; i < numEnvs_; i++) {
    auto obs = std::static_pointer_cast<SpriteObserver>(
        gdyFactory_->createObserver(grids[i], config_.observerName, 1, 0));
    obs->init({});
    obs->reset();
    observers_.push_back(obs);
  }

  auto tile = observers_[0]->getTileSize();
  envPixelWidth_ = grids[0]->getWidth() * (uint32_t)tile.x;
  envPixelHeight_ = grids[0]->getHeight() * (uint32_t)tile.y;
  pixelWidth_ = cols_ * envPixelWidth_ + (cols_ + 1) * gap_;
  pixelHeight_ = rows_ * envPixelHeight_ + (rows_ + 1) * gap_;

  pixelBuffer_.assign((size_t)pixelWidth_ * pixelHeight_ * 4, 0);

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
    spdlog::info("TiledSDLRenderer: compositing with GPU driver '{}' ({}x{} envs, frame {}x{})",
                 drv, cols_, rows_, pixelWidth_, pixelHeight_);
    break;
  }
  if (renderer_ == nullptr) {
    // Pure CPU compositing: we memcpy env buffers straight into pixelBuffer_,
    // no SDL renderer involved (SDL3 batches render commands until present,
    // which would overwrite manual memcpys).
    if (videoUp) SDL_Quit();
    videoUp = false;
    spdlog::info("TiledSDLRenderer: compositing on CPU (direct memcpy, no display needed)");
  }
}

TiledSDLRenderer::~TiledSDLRenderer() {
  for (auto& o : observers_) o->release();
  if (targetTexture_ != nullptr) SDL_DestroyTexture(targetTexture_);
  if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
  if (window_ != nullptr) SDL_DestroyWindow(window_);
  if (surface_ != nullptr) SDL_DestroySurface(surface_);
}

bool TiledSDLRenderer::isUsingGPU() const { return usingGPU_; }

const uint8_t* TiledSDLRenderer::render(bool parallel, uint32_t numThreads) {
  // On the GPU path each env observer renders straight into its own GPU target
  // texture (SDLObserver skips the per-env readback when nobody consumes its
  // pixelBuffer_). The per-env work is only render-item building (CPU) + draw
  // calls (GPU), so it parallelizes across threads; SDL render calls from
  // multiple threads on DIFFERENT renderers are safe.
  if (usingGPU_) {
    bool noReadback = std::getenv("GRIDDLY_SDL_GPU_NO_READBACK") != nullptr;
    if (parallel && numEnvs_ > 1) {
      uint32_t nt = numThreads == 0 ? std::thread::hardware_concurrency() : numThreads;
      nt = std::min({nt, numEnvs_, 32u});
      if (nt <= 1) {
        for (auto& o : observers_) o->update();
      } else {
        std::atomic<uint32_t> next{0};
        std::vector<std::thread> pool;
        pool.reserve(nt);
        for (uint32_t t = 0; t < nt; t++) {
          pool.emplace_back([&] {
            for (;;) {
              uint32_t i = next.fetch_add(1, std::memory_order_relaxed);
              if (i >= numEnvs_) return;
              observers_[i]->update();
            }
          });
        }
        for (auto& t : pool) t.join();
      }
    } else {
      for (auto& o : observers_) o->update();
    }
    // composite: blit each env's GPU target texture into the composite target.
    // SDLObserver exposes its target texture via getTargetTexture().
    SDL_SetRenderTarget(renderer_, targetTexture_);
    SDL_SetRenderDrawColor(renderer_, 20, 20, 24, 255);
    SDL_RenderClear(renderer_);
    for (uint32_t i = 0; i < numEnvs_; i++) {
      uint32_t c = i % cols_;
      uint32_t r = i / cols_;
      SDL_FRect dst{(float)(gap_ + c * (envPixelWidth_ + gap_)),
                    (float)(gap_ + r * (envPixelHeight_ + gap_)),
                    (float)envPixelWidth_, (float)envPixelHeight_};
      SDL_Texture* src = observers_[i]->getTargetTexture();
      if (src != nullptr) SDL_RenderTexture(renderer_, src, nullptr, &dst);
    }
    // single readback of the whole composite (skip when caller only needs GPU-side frame)
    SDL_Surface* shot = std::getenv("GRIDDLY_SDL_GPU_NO_READBACK") ? nullptr : SDL_RenderReadPixels(renderer_, nullptr);
    if (shot != nullptr) {
      SDL_Surface* conv = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
      if (conv != nullptr) {
        for (int y = 0; y < conv->h; y++)
          SDL_memcpy(pixelBuffer_.data() + (size_t)y * pixelWidth_ * 4,
                     (const uint8_t*)conv->pixels + (size_t)y * conv->pitch, (size_t)pixelWidth_ * 4);
        SDL_DestroySurface(conv);
      }
      SDL_DestroySurface(shot);
    }
    if (window_ != nullptr && !(SDL_GetWindowFlags(window_) & SDL_WINDOW_HIDDEN)) {
      SDL_SetRenderTarget(renderer_, nullptr);
      SDL_RenderTexture(renderer_, targetTexture_, nullptr, nullptr);
      SDL_RenderPresent(renderer_);
    }
    return pixelBuffer_.data();
  }

  // CPU path: update each env observer -> fills that observer's own pixelBuffer_
  if (parallel && numEnvs_ > 1) {
    uint32_t nt = numThreads == 0 ? std::thread::hardware_concurrency() : numThreads;
    nt = std::min({nt, numEnvs_, 32u});
    if (nt <= 1) {
      for (auto& o : observers_) o->update();
    } else {
      std::atomic<uint32_t> next{0};
      std::vector<std::thread> pool;
      pool.reserve(nt);
      for (uint32_t t = 0; t < nt; t++) {
        pool.emplace_back([&] {
          for (;;) {
            uint32_t i = next.fetch_add(1, std::memory_order_relaxed);
            if (i >= numEnvs_) return;
            observers_[i]->update();
          }
        });
      }
      for (auto& t : pool) t.join();
    }
  } else {
    for (auto& o : observers_) o->update();
  }

  // CPU path composite: clear to background, memcpy each env's rows into place.
  {
    auto* dstBase = pixelBuffer_.data();
    size_t dstRowPitch = (size_t)pixelWidth_ * 4;
    size_t rowBytes = (size_t)envPixelWidth_ * 4;
    const uint8_t bg[4] = {20, 20, 24, 255};
    for (size_t i = 0; i < (size_t)pixelWidth_ * pixelHeight_; i++) SDL_memcpy(dstBase + i * 4, bg, 4);
    for (uint32_t i = 0; i < numEnvs_; i++) {
      uint32_t c = i % cols_;
      uint32_t r = i / cols_;
      const auto& src = observers_[i]->pixelBuffer();
      if (src.size() != rowBytes * envPixelHeight_) continue;
      size_t dstOffset = (size_t)(gap_ + r * (envPixelHeight_ + gap_)) * dstRowPitch +
                         (size_t)(gap_ + c * (envPixelWidth_ + gap_)) * 4;
      for (uint32_t y = 0; y < envPixelHeight_; y++)
        SDL_memcpy(dstBase + dstOffset + (size_t)y * dstRowPitch, src.data() + (size_t)y * rowBytes, rowBytes);
    }
  }
  return pixelBuffer_.data();
}

}  // namespace griddly
