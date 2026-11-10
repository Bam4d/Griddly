#pragma once
#include <cstddef>
#include <cstdint>

// Host-only C++ interface to the CUDA tile-gather kernel so that the rest of
// Griddly never includes CUDA headers.
//
// Cell codes (uint32): bits 0-11 def+1 (0 = empty), bits 12-19 object-name
// id, bits 20-21 clockwise quarter turns. Each cell holds `layers` codes in
// ascending z order, terminated by 0.
namespace griddly::cuda {

enum : int16_t { TILING_NONE = 0, TILING_WALL_2 = 1, TILING_WALL_16 = 2 };

struct TiledKernelDesc {
  int numEnvs, gridW, gridH, tileSize, layers;
  int numSprites;
  const uint32_t* atlas;  // numSprites * tileSize^2 RGBA8 texels
  int numDefs;
  const int16_t* defTiling;  // TILING_*
  const int16_t* defBase;    // atlas index of the def's first image
  int backgroundSprite;      // -1 none
  uint32_t clearColor;       // per-env clear colour (RGBA8 little endian)
  int cols, rows, gap;
  uint32_t gapColor;
  int device;
};

class TiledKernel;

TiledKernel* createKernel(const TiledKernelDesc& desc);
void destroyKernel(TiledKernel* k);

uint32_t* hostCells(TiledKernel* k);  // pinned, numEnvs*gridH*gridW*layers
size_t outputBytes(TiledKernel* k);
uint32_t outputWidth(TiledKernel* k);
uint32_t outputHeight(TiledKernel* k);

// Upload cells + launch, asynchronously on the kernel's stream
void renderAsync(TiledKernel* k);
// Copy the composite into pinned host memory and wait; returns that buffer
const uint8_t* readback(TiledKernel* k);
// Wait for the stream; returns the device composite pointer
const uint8_t* deviceOutput(TiledKernel* k);

bool deviceAvailable();

// Synchronous device-to-host copy (for tests and tools)
void copyDeviceToHost(void* dst, const void* src, size_t bytes);

}  // namespace griddly::cuda
