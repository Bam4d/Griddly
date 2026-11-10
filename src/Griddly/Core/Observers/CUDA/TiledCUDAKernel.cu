#include <cuda_runtime.h>

#include <cstring>
#include <stdexcept>
#include <string>

#include "TiledCUDAKernel.hpp"

namespace griddly::cuda {

namespace {

void check(cudaError_t e, const char* what) {
  if (e != cudaSuccess) {
    throw std::runtime_error(std::string("CUDA ") + what + ": " + cudaGetErrorString(e));
  }
}

struct KernelParams {
  const uint32_t* cells;
  const uint32_t* atlas;
  const int16_t* defTiling;
  const int16_t* defBase;
  uint32_t* out;
  int numEnvs, gridW, gridH, tileSize, layers;
  int envW, envH;
  int backgroundSprite;
  uint32_t clearColor;
  int cols, gap, outPitch;
};

// Rotated copies are blended by SDL's per-pixel alpha blitter: (s*a + d*(255-a) + 1) * 257 >> 16
__device__ __forceinline__ uint32_t blendChannel(uint32_t s, uint32_t d, uint32_t a) {
  uint32_t x = s * a + d * (255u - a) + 1u;
  return (x + (x >> 8)) >> 8;
}

__device__ __forceinline__ uint32_t blend(uint32_t s, uint32_t d) {
  uint32_t a = s >> 24;
  if (a == 255u) return s;
  if (a == 0u) return d;
  uint32_t r = blendChannel(s & 255u, d & 255u, a);
  uint32_t g = blendChannel((s >> 8) & 255u, (d >> 8) & 255u, a);
  uint32_t b = blendChannel((s >> 16) & 255u, (d >> 16) & 255u, a);
  uint32_t outA = blendChannel(255u, d >> 24, a);
  return r | (g << 8) | (b << 16) | (outA << 24);
}

// SDL's software renderer RLE-encodes static textures, so unrotated copies go
// through SDL_RLEAlphaBlit (BLIT_TRANSL_888), which rounds differently from
// the rotated path: d + ((s - d) * a >> 8) on packed channel pairs
__device__ __forceinline__ uint32_t blendRLE(uint32_t s, uint32_t d) {
  uint32_t a = s >> 24;
  if (a == 255u) return s;
  if (a == 0u) return d;
  uint32_t s1 = s & 0xff00ffu;
  uint32_t d1 = d & 0xff00ffu;
  d1 = (d1 + ((s1 - d1) * a >> 8)) & 0xff00ffu;
  uint32_t s2 = s & 0xff00u;
  uint32_t d2 = d & 0xff00u;
  d2 = (d2 + ((s2 - d2) * a >> 8)) & 0xff00u;
  return d1 | d2 | 0xff000000u;
}

// Name id of the top-most object at (x, y), matching Grid::getObject()
__device__ __forceinline__ uint32_t topName(const uint32_t* envCells, const KernelParams& p, int x, int y) {
  if (x < 0 || y < 0 || x >= p.gridW || y >= p.gridH) return 0xFFFFFFFFu;
  const uint32_t* c = envCells + (y * p.gridW + x) * p.layers;
  uint32_t top = 0xFFFFFFFFu;
  for (int m = 0; m < p.layers; m++) {
    if (c[m] == 0) break;
    top = (c[m] >> 12) & 255u;
  }
  return top;
}

// One thread renders 4 horizontally adjacent pixels (tileSize % 4 == 0 so
// they always fall in the same cell).
__global__ void renderKernel(KernelParams p) {
  const int quadsPerRow = p.envW >> 2;
  const long long total = (long long)p.numEnvs * p.envH * quadsPerRow;
  const long long idx = (long long)blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= total) return;
  const int q = (int)(idx % quadsPerRow);
  const long long t = idx / quadsPerRow;
  const int y = (int)(t % p.envH);
  const int env = (int)(t / p.envH);
  const int x0 = q << 2;
  const int T = p.tileSize;
  const int cy = y / T, ly = y - cy * T;
  const int cx = x0 / T, lx0 = x0 - cx * T;

  const uint32_t* envCells = p.cells + (long long)env * p.gridH * p.gridW * p.layers;
  const uint32_t* cell = envCells + (cy * p.gridW + cx) * p.layers;

  uint32_t px[4];
#pragma unroll
  for (int k = 0; k < 4; k++) px[k] = p.clearColor;

  if (p.backgroundSprite >= 0) {
    const uint32_t* bg = p.atlas + (long long)p.backgroundSprite * T * T + ly * T + lx0;
#pragma unroll
    for (int k = 0; k < 4; k++) px[k] = blendRLE(__ldg(bg + k), px[k]);
  }

  for (int l = 0; l < p.layers; l++) {
    const uint32_t code = cell[l];
    if (code == 0) break;
    const int def = (int)(code & 0xFFFu) - 1;
    int rot = (code >> 20) & 3;
    int sprite = p.defBase[def];
    const int tiling = p.defTiling[def];
    if (tiling != TILING_NONE) {
      const uint32_t name = (code >> 12) & 255u;
      int wallIdx = 0;
      if (tiling == TILING_WALL_16) {
        if (topName(envCells, p, cx - 1, cy) == name) wallIdx += 1;
        if (topName(envCells, p, cx + 1, cy) == name) wallIdx += 2;
        if (topName(envCells, p, cx, cy - 1) == name) wallIdx += 4;
        if (topName(envCells, p, cx, cy + 1) == name) wallIdx += 8;
      } else {
        if (topName(envCells, p, cx, cy + 1) == name) wallIdx += 1;
      }
      sprite += wallIdx;
      rot = 0;  // tiled sprites are never rotated (SpriteObserver)
    }
    const uint32_t* spr = p.atlas + (long long)sprite * T * T;
    if (rot == 0) {
#pragma unroll
      for (int k = 0; k < 4; k++) px[k] = blendRLE(__ldg(spr + ly * T + lx0 + k), px[k]);
      continue;
    }
#pragma unroll
    for (int k = 0; k < 4; k++) {
      const int lx = lx0 + k;
      int sx, sy;
      switch (rot) {  // clockwise, SDL convention; maps destination to source texel
        case 0: sx = lx; sy = ly; break;
        case 1: sx = ly; sy = T - 1 - lx; break;
        case 2: sx = T - 1 - lx; sy = T - 1 - ly; break;
        default: sx = T - 1 - ly; sy = lx; break;
      }
      px[k] = blend(__ldg(spr + sy * T + sx), px[k]);
    }
  }

  const int c = env % p.cols, r = env / p.cols;
  uint32_t* o = p.out + (long long)(p.gap + r * (p.envH + p.gap) + y) * p.outPitch + p.gap + c * (p.envW + p.gap) + x0;
  o[0] = px[0];
  o[1] = px[1];
  o[2] = px[2];
  o[3] = px[3];
}

__global__ void fillKernel(uint32_t* out, size_t n, uint32_t v) {
  size_t i = (size_t)blockIdx.x * blockDim.x + threadIdx.x;
  if (i < n) out[i] = v;
}

}  // namespace

class TiledKernel {
 public:
  explicit TiledKernel(const TiledKernelDesc& d) : device_(d.device) {
    if (d.tileSize % 4 != 0) throw std::invalid_argument("CUDA tiled renderer requires a tile size that is a multiple of 4");
    if (d.numDefs > 4095) throw std::invalid_argument("CUDA tiled renderer supports at most 4095 sprite definitions");
    check(cudaSetDevice(device_), "cudaSetDevice");
    check(cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking), "stream create");

    p_.numEnvs = d.numEnvs;
    p_.gridW = d.gridW;
    p_.gridH = d.gridH;
    p_.tileSize = d.tileSize;
    p_.layers = d.layers;
    p_.envW = d.gridW * d.tileSize;
    p_.envH = d.gridH * d.tileSize;
    p_.backgroundSprite = d.backgroundSprite;
    p_.clearColor = d.clearColor;
    p_.cols = d.cols;
    p_.gap = d.gap;

    width_ = (uint32_t)(d.cols * p_.envW + (d.cols + 1) * d.gap);
    height_ = (uint32_t)(d.rows * p_.envH + (d.rows + 1) * d.gap);
    p_.outPitch = (int)width_;
    outPixels_ = (size_t)width_ * height_;

    size_t atlasN = (size_t)d.numSprites * d.tileSize * d.tileSize;
    cellsN_ = (size_t)d.numEnvs * d.gridW * d.gridH * d.layers;
    check(cudaMalloc(&dAtlas_, atlasN * 4), "malloc atlas");
    check(cudaMalloc(&dDefTiling_, sizeof(int16_t) * d.numDefs), "malloc defs");
    check(cudaMalloc(&dDefBase_, sizeof(int16_t) * d.numDefs), "malloc defs");
    check(cudaMalloc(&dCells_, cellsN_ * 4), "malloc cells");
    check(cudaMalloc(&dOut_, outPixels_ * 4), "malloc output");
    check(cudaMallocHost(&hCells_, cellsN_ * 4), "malloc host cells");
    check(cudaMallocHost(&hOut_, outPixels_ * 4), "malloc host output");
    std::memset(hCells_, 0, cellsN_ * 4);

    check(cudaMemcpy(dAtlas_, d.atlas, atlasN * 4, cudaMemcpyHostToDevice), "upload atlas");
    check(cudaMemcpy(dDefTiling_, d.defTiling, sizeof(int16_t) * d.numDefs, cudaMemcpyHostToDevice), "upload defs");
    check(cudaMemcpy(dDefBase_, d.defBase, sizeof(int16_t) * d.numDefs, cudaMemcpyHostToDevice), "upload defs");

    p_.cells = dCells_;
    p_.atlas = dAtlas_;
    p_.defTiling = dDefTiling_;
    p_.defBase = dDefBase_;
    p_.out = dOut_;

    // gaps never change, fill them once; env views are fully overwritten every frame
    fillKernel<<<(unsigned)((outPixels_ + 255) / 256), 256, 0, stream_>>>(dOut_, outPixels_, d.gapColor);
    check(cudaGetLastError(), "fill launch");
    check(cudaStreamSynchronize(stream_), "fill sync");
  }

  ~TiledKernel() {
    cudaSetDevice(device_);
    if (stream_ != nullptr) cudaStreamSynchronize(stream_);
    cudaFree(dAtlas_);
    cudaFree(dDefTiling_);
    cudaFree(dDefBase_);
    cudaFree(dCells_);
    cudaFree(dOut_);
    cudaFreeHost(hCells_);
    cudaFreeHost(hOut_);
    if (stream_ != nullptr) cudaStreamDestroy(stream_);
  }

  void renderAsync() {
    check(cudaSetDevice(device_), "cudaSetDevice");
    check(cudaMemcpyAsync(dCells_, hCells_, cellsN_ * 4, cudaMemcpyHostToDevice, stream_), "upload cells");
    long long total = (long long)p_.numEnvs * p_.envH * (p_.envW >> 2);
    unsigned blocks = (unsigned)((total + 255) / 256);
    renderKernel<<<blocks, 256, 0, stream_>>>(p_);
    check(cudaGetLastError(), "render launch");
  }

  const uint8_t* readback() {
    check(cudaMemcpyAsync(hOut_, dOut_, outPixels_ * 4, cudaMemcpyDeviceToHost, stream_), "readback");
    check(cudaStreamSynchronize(stream_), "readback sync");
    return hOut_;
  }

  const uint8_t* deviceOutput() {
    check(cudaStreamSynchronize(stream_), "render sync");
    return reinterpret_cast<const uint8_t*>(dOut_);
  }

  uint32_t* hostCells() { return hCells_; }
  size_t outputBytes() const { return outPixels_ * 4; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

 private:
  int device_ = 0;
  KernelParams p_{};
  cudaStream_t stream_ = nullptr;
  uint32_t* dAtlas_ = nullptr;
  int16_t* dDefTiling_ = nullptr;
  int16_t* dDefBase_ = nullptr;
  uint32_t* dCells_ = nullptr;
  uint32_t* dOut_ = nullptr;
  uint32_t* hCells_ = nullptr;
  uint8_t* hOut_ = nullptr;
  size_t cellsN_ = 0, outPixels_ = 0;
  uint32_t width_ = 0, height_ = 0;
};

TiledKernel* createKernel(const TiledKernelDesc& desc) { return new TiledKernel(desc); }
void destroyKernel(TiledKernel* k) { delete k; }
uint32_t* hostCells(TiledKernel* k) { return k->hostCells(); }
size_t outputBytes(TiledKernel* k) { return k->outputBytes(); }
uint32_t outputWidth(TiledKernel* k) { return k->width(); }
uint32_t outputHeight(TiledKernel* k) { return k->height(); }
void renderAsync(TiledKernel* k) { k->renderAsync(); }
const uint8_t* readback(TiledKernel* k) { return k->readback(); }
const uint8_t* deviceOutput(TiledKernel* k) { return k->deviceOutput(); }

void copyDeviceToHost(void* dst, const void* src, size_t bytes) {
  check(cudaMemcpy(dst, src, bytes, cudaMemcpyDeviceToHost), "device to host copy");
}

bool deviceAvailable() {
  int n = 0;
  if (cudaGetDeviceCount(&n) != cudaSuccess) {
    cudaGetLastError();
    return false;
  }
  return n > 0;
}

}  // namespace griddly::cuda
