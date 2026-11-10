#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

namespace griddly {

class GDYFactory;
class Grid;

enum class TiledRenderBackend {
  // CUDA when compiled in, a device is present and the observer is supported; SDL otherwise
  AUTO,
  // SDL observers per env: full feature set, the place for 3D/effects work
  SDL,
  // CUDA tile-gather kernel: Sprite2D only, much faster for large env counts
  CUDA,
};

struct TiledRendererConfig {
  // Which named observer (from the GDY) to use for each env's sub-view
  std::string observerName = "Sprite2D";
  // Grid layout of the tiled output: cols x rows tiles of envs.
  // 0 = auto (square-ish). The compositor sizes itself to cols*envW x rows*envH.
  uint32_t cols = 0;
  uint32_t rows = 0;
  // Gap in pixels between env tiles
  uint32_t gap = 2;
  // GRIDDLY_TILED_BACKEND=sdl|cuda|auto overrides AUTO
  TiledRenderBackend backend = TiledRenderBackend::AUTO;
  int cudaDevice = 0;
};

// Colour of the borders and unused layout slots: RGBA {20, 20, 24, 255}
// (packed little-endian, byte 0 = R)
constexpr uint8_t kTiledGapRGBA[4] = {20, 20, 24, 255};
constexpr uint32_t kTiledGapColor = 0xFF181414u;

// Renders N envs into one RGBA8 image laid out in a cols x rows grid with
// `gap` pixel borders of kTiledGapColor.
class TiledRenderer {
 public:
  TiledRenderer() = default;
  virtual ~TiledRenderer();
  TiledRenderer(const TiledRenderer&) = delete;
  TiledRenderer& operator=(const TiledRenderer&) = delete;

  // Render all envs and return the host RGBA8 composite (width()*height()*4 bytes)
  virtual const uint8_t* render(bool parallel = true, uint32_t numThreads = 0) = 0;

  // Device-resident composite if the backend has one (CUDA), else nullptr.
  // renderDevice() skips the host readback; the pointer stays valid until the
  // next render call.
  virtual const uint8_t* renderDevice(bool parallel = true, uint32_t numThreads = 0) { (void)parallel; (void)numThreads; return nullptr; }
  virtual int deviceId() const { return -1; }

  virtual TiledRenderBackend backend() const = 0;
  virtual bool isUsingGPU() const = 0;

  uint32_t width() const { return pixelWidth_; }
  uint32_t height() const { return pixelHeight_; }
  uint32_t envPixelWidth() const { return envPixelWidth_; }
  uint32_t envPixelHeight() const { return envPixelHeight_; }
  uint32_t cols() const { return cols_; }
  uint32_t rows() const { return rows_; }
  // Pixel offset of env i's top-left corner in the composite
  uint32_t envOffsetX(uint32_t i) const { return gap_ + (i % cols_) * (envPixelWidth_ + gap_); }
  uint32_t envOffsetY(uint32_t i) const { return gap_ + (i / cols_) * (envPixelHeight_ + gap_); }

 protected:
  void computeLayout(uint32_t numEnvs, const TiledRendererConfig& config, uint32_t envPixelWidth, uint32_t envPixelHeight);

  // Run fn(i) for i in [0, count) on a persistent worker pool of up to
  // numThreads threads (0 = hardware concurrency, capped at 32), with at
  // least minPerThread items per thread. Blocks until all items are done.
  void parallelFor(uint32_t count, uint32_t numThreads, uint32_t minPerThread, const std::function<void(uint32_t)>& fn);

  uint32_t numEnvs_ = 0;
  uint32_t cols_ = 0, rows_ = 0;
  uint32_t envPixelWidth_ = 0, envPixelHeight_ = 0;
  uint32_t pixelWidth_ = 0, pixelHeight_ = 0;
  uint32_t gap_ = 2;

 private:
  void workerLoop(uint32_t workerIdx);
  void stopWorkers();

  std::vector<std::thread> workers_;
  std::mutex poolMutex_;
  std::condition_variable poolWork_, poolDone_;
  const std::function<void(uint32_t)>* poolFn_ = nullptr;
  uint32_t poolCount_ = 0, poolActiveWorkers_ = 0, poolRunning_ = 0;
  uint64_t poolGeneration_ = 0;
  uint32_t poolNext_ = 0;
  bool poolStop_ = false;
};

// True when the CUDA backend is compiled in and a CUDA device is visible
bool tiledCUDAAvailable();

// Builds the backend selected by config.backend (or GRIDDLY_TILED_BACKEND).
// AUTO falls back to SDL if CUDA is unavailable or the observer uses
// features the CUDA kernel does not implement.
std::unique_ptr<TiledRenderer> createTiledRenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                                                   const std::vector<std::shared_ptr<Grid>>& grids,
                                                   TiledRendererConfig config);

}  // namespace griddly
