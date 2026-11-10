#pragma once
#include <SDL3/SDL.h>

#include <memory>
#include <vector>

#include "SpriteObserver.hpp"
#include "TiledRenderer.hpp"

namespace griddly {

// Kept for source compatibility; the backend field is ignored by TiledSDLRenderer
using TiledSDLRendererConfig = TiledRendererConfig;

// Renders N per-env SDL observers into one image laid out in a cols x rows grid.
//
// GPU: one renderer and one composite target texture. All observers share
// the renderer and its sprite textures and draw their tile through a
// viewport; render items are built on the worker pool, draw calls are issued
// from the calling thread (GL contexts are single-threaded), then the
// composite is read back once.
//
// CPU: each observer's software renderer draws straight into its rect of the
// composite buffer, fully in parallel on the worker pool.
class TiledSDLRenderer : public TiledRenderer {
 public:
  TiledSDLRenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                   const std::vector<std::shared_ptr<Grid>>& grids,
                   TiledSDLRendererConfig config);
  ~TiledSDLRenderer() override;

  TiledSDLRenderer(const TiledSDLRenderer&) = delete;
  TiledSDLRenderer& operator=(const TiledSDLRenderer&) = delete;

  // Per-env observer
  SDLObserver& envObserver(uint32_t i) { return *observers_.at(i); }

  // Render all envs (optionally on a thread pool) and composite into
  // pixelBuffer_. Returns a pointer to the RGBA8 buffer.
  const uint8_t* render(bool parallel = true, uint32_t numThreads = 0) override;

  const std::vector<uint8_t>& buffer() const { return pixelBuffer_; }

  bool isUsingGPU() const override;
  TiledRenderBackend backend() const override { return TiledRenderBackend::SDL; }

 private:
  void readbackComposite();
  // Renders clear colour + background once and hands it to every observer
  void buildBaseLayer();
  // GPU: true when every prepared item of observer i sits inside one tile
  // cell of its env (axis aligned, quarter-turn rotations only), so its draw
  // order only matters against items of the same env and cell.
  bool isBatchable(uint32_t i) const;
  // GPU: draw the prepared items of the given envs as a few SDL_RenderGeometry
  // calls, one per (z, texture) run across all envs
  void drawBatched(const std::vector<uint32_t>& envs);

  using GLReadPixelsFn = void (*)(int, int, int, int, unsigned int, unsigned int, void*);
  GLReadPixelsFn glReadPixels_ = nullptr;
  using GLFinishFn = void (*)();
  GLFinishFn glFinish_ = nullptr;

  std::shared_ptr<GDYFactory> gdyFactory_;
  TiledSDLRendererConfig config_;

  std::vector<std::shared_ptr<SDLObserver>> observers_;
  SpriteTextureMap sharedSprites_;  // GPU: textures of renderer_, shared by all observers
  std::vector<uint8_t> basePixels_;     // CPU: one env's clear + background, RGBA8
  SDL_Texture* baseTexture_ = nullptr;  // GPU: same, as a texture of renderer_
  bool baseLayerBuilt_ = false;

  // GPU batching scratch, reused every frame
  struct BatchQuad {
    float z;
    SDL_Texture* texture;
    uint32_t env;
    uint32_t item;
  };
  std::vector<BatchQuad> batchQuads_;
  std::vector<SDL_Vertex> batchVertices_;
  std::vector<int> batchIndices_;
  std::vector<uint32_t> batchEnvs_, fallbackEnvs_;

  // composite target
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* targetTexture_ = nullptr;
  SDL_Surface* surface_ = nullptr;
  std::vector<uint8_t> pixelBuffer_;
  bool usingGPU_ = false;
};

}  // namespace griddly
