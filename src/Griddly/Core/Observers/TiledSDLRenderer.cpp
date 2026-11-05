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

  // Composite target: try GPU first, fall back to pure-CPU software surface
  const char* driver = nullptr;
  bool videoUp = false;
  if (std::getenv("GRIDDLY_SDL_FORCE_CPU") == nullptr && SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    videoUp = true;
    int driverCount = SDL_GetNumRenderDrivers();
    for (int i = 0; i < driverCount; i++) {
      const char* name = SDL_GetRenderDriver(i);
      if (name != nullptr && SDL_strcasecmp(name, "software") != 0) { driver = name; break; }
    }
  }
  if (driver != nullptr) {
    window_ = SDL_CreateWindow("Griddly Tiled", (int)pixelWidth_, (int)pixelHeight_, SDL_WINDOW_HIDDEN);
    if (window_ != nullptr) {
      renderer_ = SDL_CreateRenderer(window_, driver);
      if (renderer_ != nullptr) {
        usingGPU_ = true;
        targetTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                            (int)pixelWidth_, (int)pixelHeight_);
        spdlog::info("TiledSDLRenderer: compositing with GPU driver '{}' ({}x{} envs, frame {}x{})",
                     driver, cols_, rows_, pixelWidth_, pixelHeight_);
      }
    }
  }
  if (renderer_ == nullptr) {
    if (videoUp) SDL_QuitSubSystem(SDL_INIT_VIDEO);  // redo cleanly below
    spdlog::info("TiledSDLRenderer: no GPU driver, compositing on CPU (software, no display needed)");
    surface_ = SDL_CreateSurfaceFrom((int)pixelWidth_, (int)pixelHeight_, SDL_PIXELFORMAT_RGBA32,
                                     pixelBuffer_.data(), (int)(pixelWidth_ * 4));
    if (surface_ == nullptr) throw std::runtime_error("TiledSDLRenderer: failed to create surface");
    renderer_ = SDL_CreateSoftwareRenderer(surface_);
    if (renderer_ == nullptr) throw std::runtime_error("TiledSDLRenderer: failed to create software renderer");
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
  // 1) update each env observer -> fills that observer's own pixelBuffer_
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

  compositeFrame();
  return pixelBuffer_.data();
}

void TiledSDLRenderer::compositeFrame() {
  SDL_SetRenderTarget(renderer_, usingGPU_ ? targetTexture_ : nullptr);
  SDL_SetRenderDrawColor(renderer_, 20, 20, 24, 255);
  SDL_RenderClear(renderer_);

  for (uint32_t i = 0; i < numEnvs_; i++) {
    uint32_t c = i % cols_;
    uint32_t r = i / cols_;
    SDL_FRect dst{(float)(gap_ + c * (envPixelWidth_ + gap_)),
                  (float)(gap_ + r * (envPixelHeight_ + gap_)),
                  (float)envPixelWidth_, (float)envPixelHeight_};

    const auto& src = observers_[i]->pixelBuffer();
    if (src.size() != (size_t)envPixelWidth_ * envPixelHeight_ * 4) continue;

    if (usingGPU_) {
      // upload once per frame (small frames; fine for now)
      SDL_Texture* tex = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                           (int)envPixelWidth_, (int)envPixelHeight_);
      if (tex != nullptr) {
        SDL_UpdateTexture(tex, nullptr, src.data(), (int)(envPixelWidth_ * 4));
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
        SDL_RenderTexture(renderer_, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
      }
    } else {
      // CPU path: straight memcpy rows into the composite buffer
      auto* dstBase = pixelBuffer_.data();
      size_t rowBytes = (size_t)envPixelWidth_ * 4;
      size_t dstRowPitch = (size_t)pixelWidth_ * 4;
      size_t dstOffset = (size_t)(dst.y) * dstRowPitch + (size_t)(dst.x) * 4;
      for (uint32_t y = 0; y < envPixelHeight_; y++) {
        SDL_memcpy(dstBase + dstOffset + (size_t)y * dstRowPitch,
                   src.data() + (size_t)y * rowBytes, rowBytes);
      }
    }
  }

  if (usingGPU_) {
    SDL_Surface* shot = SDL_RenderReadPixels(renderer_, nullptr);
    if (shot != nullptr) {
      SDL_Surface* conv = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
      if (conv != nullptr) {
        for (int y = 0; y < conv->h; y++) {
          SDL_memcpy(pixelBuffer_.data() + (size_t)y * pixelWidth_ * 4,
                     (const uint8_t*)conv->pixels + (size_t)y * conv->pitch,
                     (size_t)pixelWidth_ * 4);
        }
        SDL_DestroySurface(conv);
      }
      SDL_DestroySurface(shot);
    }
    if (window_ != nullptr && !(SDL_GetWindowFlags(window_) & SDL_WINDOW_HIDDEN)) {
      SDL_SetRenderTarget(renderer_, nullptr);
      SDL_RenderTexture(renderer_, targetTexture_, nullptr, nullptr);
      SDL_RenderPresent(renderer_);
    }
  } else {
    SDL_RenderPresent(renderer_);  // software renderer flushes into pixelBuffer_
  }
}

}  // namespace griddly
