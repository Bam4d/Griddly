#pragma once
#include <SDL3/SDL.h>

#include <memory>
#include <vector>

#include "SpriteObserver.hpp"

namespace griddly {

class GDYFactory;

struct TiledSDLRendererConfig {
  // Which named observer (from the GDY) to use for each env's sub-view
  std::string observerName = "Sprite2D";
  // Grid layout of the tiled output: cols x rows tiles of envs.
  // 0 = auto (square-ish). The compositor sizes itself to cols*envW x rows*envH.
  uint32_t cols = 0;
  uint32_t rows = 0;
  // Gap in pixels between env tiles
  uint32_t gap = 2;
};

// Composites N per-env SpriteObservers into one SDL surface, laid out in a
// cols x rows grid. Rendering the N sub-views can happen on a thread pool
// (renderEnvsParallel); compositing is a straight memcpy per env rect into
// the shared pixel buffer (CPU path) or N blits (GPU path).
//
// All SDL calls must happen on one thread, so renderEnvsParallel only
// prepares per-env CPU pixel buffers (SDLObserver already renders into its
// own pixelBuffer_ via the software renderer when no GPU is present); the
// actual blit into the composite happens on the calling thread.
class TiledSDLRenderer {
 public:
  TiledSDLRenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                   const std::vector<std::shared_ptr<Grid>>& grids,
                   TiledSDLRendererConfig config);
  ~TiledSDLRenderer();

  TiledSDLRenderer(const TiledSDLRenderer&) = delete;
  TiledSDLRenderer& operator=(const TiledSDLRenderer&) = delete;

  uint32_t width() const { return pixelWidth_; }
  uint32_t height() const { return pixelHeight_; }
  uint32_t envPixelWidth() const { return envPixelWidth_; }
  uint32_t envPixelHeight() const { return envPixelHeight_; }
  uint32_t cols() const { return cols_; }
  uint32_t rows() const { return rows_; }

  // Per-env observer (for update() after each step)
  SpriteObserver& envObserver(uint32_t i) { return *observers_.at(i); }

  // Render all envs (optionally on a thread pool) and composite into
  // pixelBuffer_. Returns a pointer to the RGBA8 buffer.
  const uint8_t* render(bool parallel = true, uint32_t numThreads = 0);

  const std::vector<uint8_t>& buffer() const { return pixelBuffer_; }

  bool isUsingGPU() const;

 private:

  std::shared_ptr<GDYFactory> gdyFactory_;
  TiledSDLRendererConfig config_;
  uint32_t numEnvs_;
  uint32_t cols_ = 0, rows_ = 0;
  uint32_t envPixelWidth_ = 0, envPixelHeight_ = 0;
  uint32_t pixelWidth_ = 0, pixelHeight_ = 0;
  uint32_t gap_ = 2;

  std::vector<std::shared_ptr<SpriteObserver>> observers_;

  // composite target
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* targetTexture_ = nullptr;
  SDL_Surface* surface_ = nullptr;
  std::vector<uint8_t> pixelBuffer_;
  bool usingGPU_ = false;
};

}  // namespace griddly
