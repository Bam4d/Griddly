#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../TiledRenderer.hpp"

namespace griddly {

class GDYFactory;
class Grid;

namespace cuda {
class TiledKernel;
}

// Sprite2D tiled renderer that rasterises every env in one CUDA kernel.
//
// The CPU only writes a small per-cell code buffer (sprite def, object name,
// rotation for up to `layers` stacked objects); the kernel gathers texels from
// a sprite atlas, applies WALL_2/WALL_16 tiling, rotation and SDL's alpha
// blend, and writes the composite directly. Output is pixel-identical to the
// software TiledSDLRenderer for the supported feature set; anything else
// (avatar tracking, offsets, scaled sprites, highlights, isometric, ...) must
// use the SDL backend, see unsupportedReason().
class TiledCUDARenderer : public TiledRenderer {
 public:
  TiledCUDARenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                    const std::vector<std::shared_ptr<Grid>>& grids,
                    TiledRendererConfig config);
  ~TiledCUDARenderer() override;

  TiledCUDARenderer(const TiledCUDARenderer&) = delete;
  TiledCUDARenderer& operator=(const TiledCUDARenderer&) = delete;

  const uint8_t* render(bool parallel = true, uint32_t numThreads = 0) override;
  const uint8_t* renderDevice(bool parallel = true, uint32_t numThreads = 0) override;
  int deviceId() const override { return config_.cudaDevice; }

  TiledRenderBackend backend() const override { return TiledRenderBackend::CUDA; }
  bool isUsingGPU() const override { return true; }

  // Empty if the named observer can be rendered by this backend, otherwise why not
  static std::string unsupportedReason(const std::shared_ptr<GDYFactory>& gdyFactory, const std::shared_ptr<Grid>& grid,
                                       const std::string& observerName);

 private:
  struct SpriteDef {
    int16_t tiling;
    int16_t atlasBase;
  };

  void buildAtlas(const std::string& imagePath, uint32_t tileSize);
  void createKernel(uint32_t layers);
  // Writes cell codes for all envs; returns the deepest object stack seen
  uint32_t fillCells(bool parallel, uint32_t numThreads);
  uint32_t fillEnv(uint32_t envIdx);
  void prepareFrame(bool parallel, uint32_t numThreads);

  std::shared_ptr<GDYFactory> gdyFactory_;
  TiledRendererConfig config_;
  std::vector<std::shared_ptr<Grid>> grids_;
  uint32_t gridWidth_ = 0, gridHeight_ = 0, tileSize_ = 0;
  bool rotateSprites_ = true;
  uint32_t clearColor_ = 0xFF000000u;

  std::vector<uint32_t> atlas_;
  std::vector<int16_t> defTiling_, defBase_;
  std::unordered_map<std::string, int> defIndex_;   // render tile name -> def
  std::unordered_map<std::string, uint32_t> nameIds_;  // object name -> id
  int backgroundSprite_ = -1;

  uint32_t layers_ = 0;
  cuda::TiledKernel* kernel_ = nullptr;
};

}  // namespace griddly
