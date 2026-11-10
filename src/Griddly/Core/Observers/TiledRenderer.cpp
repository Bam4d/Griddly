#include "TiledRenderer.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "TiledSDLRenderer.hpp"
#ifdef GRIDDLY_HAS_CUDA
#include "CUDA/TiledCUDAKernel.hpp"
#include "CUDA/TiledCUDARenderer.hpp"
#endif

namespace griddly {

void TiledRenderer::computeLayout(uint32_t numEnvs, const TiledRendererConfig& config, uint32_t envPixelWidth, uint32_t envPixelHeight) {
  numEnvs_ = numEnvs;
  cols_ = config.cols > 0 ? config.cols : static_cast<uint32_t>(std::ceil(std::sqrt(static_cast<double>(numEnvs))));
  rows_ = config.rows > 0 ? config.rows : (numEnvs + cols_ - 1) / cols_;
  if (cols_ * rows_ < numEnvs) throw std::invalid_argument("tiled renderer layout cols*rows is smaller than the number of envs");
  gap_ = config.gap;
  envPixelWidth_ = envPixelWidth;
  envPixelHeight_ = envPixelHeight;
  pixelWidth_ = cols_ * envPixelWidth_ + (cols_ + 1) * gap_;
  pixelHeight_ = rows_ * envPixelHeight_ + (rows_ + 1) * gap_;
}

TiledRenderer::~TiledRenderer() {
  stopWorkers();
}

void TiledRenderer::stopWorkers() {
  {
    std::lock_guard<std::mutex> lock(poolMutex_);
    poolStop_ = true;
  }
  poolWork_.notify_all();
  for (auto& w : workers_) w.join();
  workers_.clear();
  poolStop_ = false;
}

void TiledRenderer::workerLoop(uint32_t workerIdx) {
  uint64_t seenGeneration = 0;
  for (;;) {
    const std::function<void(uint32_t)>* fn;
    {
      std::unique_lock<std::mutex> lock(poolMutex_);
      poolWork_.wait(lock, [&] { return poolStop_ || (poolGeneration_ != seenGeneration && workerIdx < poolActiveWorkers_); });
      if (poolStop_) return;
      seenGeneration = poolGeneration_;
      fn = poolFn_;
    }
    for (;;) {
      uint32_t i;
      {
        std::lock_guard<std::mutex> lock(poolMutex_);
        if (poolNext_ >= poolCount_) break;
        i = poolNext_++;
      }
      (*fn)(i);
    }
    {
      std::lock_guard<std::mutex> lock(poolMutex_);
      if (--poolRunning_ == 0) poolDone_.notify_one();
    }
  }
}

void TiledRenderer::parallelFor(uint32_t count, uint32_t numThreads, uint32_t minPerThread, const std::function<void(uint32_t)>& fn) {
  uint32_t nt = numThreads == 0 ? std::thread::hardware_concurrency() : numThreads;
  nt = std::min({nt, 32u, (count + std::max(minPerThread, 1u) - 1) / std::max(minPerThread, 1u)});
  if (nt <= 1) {
    for (uint32_t i = 0; i < count; i++) fn(i);
    return;
  }
  while (workers_.size() < nt) {
    workers_.emplace_back([this, idx = static_cast<uint32_t>(workers_.size())] { workerLoop(idx); });
  }
  std::unique_lock<std::mutex> lock(poolMutex_);
  poolFn_ = &fn;
  poolCount_ = count;
  poolNext_ = 0;
  poolActiveWorkers_ = nt;
  poolRunning_ = nt;
  poolGeneration_++;
  poolWork_.notify_all();
  poolDone_.wait(lock, [&] { return poolRunning_ == 0; });
  poolFn_ = nullptr;
}

bool tiledCUDAAvailable() {
#ifdef GRIDDLY_HAS_CUDA
  static const bool available = cuda::deviceAvailable();
  return available;
#else
  return false;
#endif
}

std::unique_ptr<TiledRenderer> createTiledRenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                                                   const std::vector<std::shared_ptr<Grid>>& grids,
                                                   TiledRendererConfig config) {
  auto backend = config.backend;
  if (const char* env = std::getenv("GRIDDLY_TILED_BACKEND")) {
    std::string value(env);
    if (value == "sdl") backend = TiledRenderBackend::SDL;
    else if (value == "cuda") backend = TiledRenderBackend::CUDA;
    else if (value == "auto") backend = TiledRenderBackend::AUTO;
    else spdlog::warn("Ignoring unknown GRIDDLY_TILED_BACKEND '{}' (expected sdl, cuda or auto)", value);
  }

  if (backend != TiledRenderBackend::SDL) {
#ifdef GRIDDLY_HAS_CUDA
    if (grids.empty()) throw std::invalid_argument("tiled renderer requires >= 1 env");
    auto reason = TiledCUDARenderer::unsupportedReason(gdyFactory, grids[0], config.observerName);
    if (reason.empty()) {
      return std::make_unique<TiledCUDARenderer>(gdyFactory, numEnvs, grids, config);
    }
    if (backend == TiledRenderBackend::CUDA) throw std::invalid_argument("CUDA tiled renderer unavailable: " + reason);
    spdlog::info("Tiled renderer: using SDL backend ({})", reason);
#else
    if (backend == TiledRenderBackend::CUDA) throw std::invalid_argument("CUDA tiled renderer unavailable: Griddly was built without CUDA");
#endif
  }
  return std::make_unique<TiledSDLRenderer>(gdyFactory, numEnvs, grids, config);
}

}  // namespace griddly
