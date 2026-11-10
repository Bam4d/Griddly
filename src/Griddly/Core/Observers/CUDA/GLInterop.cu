#include <cuda_gl_interop.h>
#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

#include "GLInterop.hpp"

namespace griddly::cuda {

namespace {

void check(cudaError_t e, const char* what) {
  if (e != cudaSuccess) {
    throw std::runtime_error(std::string("CUDA GL interop ") + what + ": " + cudaGetErrorString(e));
  }
}

constexpr unsigned int kGLTexture2D = 0x0DE1;  // GL_TEXTURE_2D

// One block per rect: copy the texels of each rect from the mapped GL
// texture (the GL RGBA8 array has 4 x 8-bit channels) into the linear buffer
__global__ void copyRectsKernel(cudaTextureObject_t texture, const GLRect* rects, uchar4* out, uint32_t pitch) {
  const GLRect r = rects[blockIdx.x];
  const uint32_t n = r.w * r.h;
  for (uint32_t i = threadIdx.x; i < n; i += blockDim.x) {
    const uint32_t x = r.x + i % r.w, y = r.y + i / r.w;
    out[static_cast<size_t>(y) * pitch + x] = tex2D<uchar4>(texture, static_cast<int>(x), static_cast<int>(y));
  }
}

}  // namespace

class GLTextureShare {
 public:
  GLTextureShare(unsigned int glTexture, uint32_t width, uint32_t height) : width_(width), height_(height) {
    // cudaGLGetDevices is not supported for EGL contexts, so try registering
    // on each CUDA device until one accepts the texture
    int deviceCount = 0;
    check(cudaGetDeviceCount(&deviceCount), "cudaGetDeviceCount");
    cudaError_t lastError = cudaErrorNoDevice;
    for (int d = 0; d < deviceCount && resource_ == nullptr; d++) {
      check(cudaSetDevice(d), "cudaSetDevice");
      lastError = cudaGraphicsGLRegisterImage(&resource_, glTexture, kGLTexture2D, cudaGraphicsRegisterFlagsReadOnly);
      if (lastError == cudaSuccess) {
        device_ = d;
      } else {
        resource_ = nullptr;
        cudaGetLastError();
      }
    }
    check(lastError, "register texture");
    check(cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking), "stream create");
    check(cudaMalloc(&buffer_, static_cast<size_t>(width) * height * 4), "malloc");
  }

  ~GLTextureShare() {
    cudaSetDevice(device_);
    if (resource_ != nullptr) cudaGraphicsUnregisterResource(resource_);
    cudaFree(buffer_);
    cudaFree(deviceRects_);
    if (stream_ != nullptr) cudaStreamDestroy(stream_);
  }

  const uint8_t* copyRects(const GLRect* rects, size_t count) {
    if (count == 0) return buffer_;
    check(cudaSetDevice(device_), "cudaSetDevice");
    if (count > rectCapacity_) {
      cudaFree(deviceRects_);
      rectCapacity_ = count * 2;
      check(cudaMalloc(&deviceRects_, rectCapacity_ * sizeof(GLRect)), "malloc rects");
    }
    check(cudaMemcpyAsync(deviceRects_, rects, count * sizeof(GLRect), cudaMemcpyHostToDevice, stream_), "upload rects");
    check(cudaGraphicsMapResources(1, &resource_, stream_), "map");
    cudaArray_t array = nullptr;
    cudaError_t e = cudaGraphicsSubResourceGetMappedArray(&array, resource_, 0, 0);
    cudaTextureObject_t texture = 0;
    if (e == cudaSuccess) {
      cudaResourceDesc res{};
      res.resType = cudaResourceTypeArray;
      res.res.array.array = array;
      cudaTextureDesc desc{};
      desc.addressMode[0] = desc.addressMode[1] = cudaAddressModeClamp;
      desc.filterMode = cudaFilterModePoint;
      desc.readMode = cudaReadModeElementType;
      e = cudaCreateTextureObject(&texture, &res, &desc, nullptr);
    }
    if (e == cudaSuccess) {
      copyRectsKernel<<<static_cast<unsigned>(count), 256, 0, stream_>>>(texture, deviceRects_, reinterpret_cast<uchar4*>(buffer_), width_);
      e = cudaGetLastError();
    }
    cudaError_t unmap = cudaGraphicsUnmapResources(1, &resource_, stream_);
    if (texture != 0) cudaDestroyTextureObject(texture);
    check(e, "copy rects");
    check(unmap, "unmap");
    check(cudaStreamSynchronize(stream_), "sync");
    return buffer_;
  }

  const uint8_t* copy() {
    check(cudaSetDevice(device_), "cudaSetDevice");
    check(cudaGraphicsMapResources(1, &resource_, stream_), "map");
    cudaArray_t array = nullptr;
    cudaError_t e = cudaGraphicsSubResourceGetMappedArray(&array, resource_, 0, 0);
    if (e == cudaSuccess) {
      // GL texture rows are bottom-up only for the default framebuffer; render
      // target textures store row 0 = the first row SDL drew (the top)
      const size_t rowBytes = static_cast<size_t>(width_) * 4;
      e = cudaMemcpy2DFromArrayAsync(buffer_, rowBytes, array, 0, 0, rowBytes, height_, cudaMemcpyDeviceToDevice, stream_);
    }
    cudaError_t unmap = cudaGraphicsUnmapResources(1, &resource_, stream_);
    check(e, "copy");
    check(unmap, "unmap");
    check(cudaStreamSynchronize(stream_), "sync");
    return buffer_;
  }

  int device() const { return device_; }

 private:
  uint32_t width_, height_;
  int device_ = 0;
  cudaStream_t stream_ = nullptr;
  cudaGraphicsResource_t resource_ = nullptr;
  uint8_t* buffer_ = nullptr;
  GLRect* deviceRects_ = nullptr;
  size_t rectCapacity_ = 0;
};

GLTextureShare* createGLTextureShare(unsigned int glTexture, uint32_t width, uint32_t height) {
  return new GLTextureShare(glTexture, width, height);
}
void destroyGLTextureShare(GLTextureShare* share) { delete share; }
const uint8_t* copyGLTexture(GLTextureShare* share) { return share->copy(); }
const uint8_t* copyGLTextureRects(GLTextureShare* share, const GLRect* rects, size_t count) { return share->copyRects(rects, count); }
int glTextureShareDevice(const GLTextureShare* share) { return share->device(); }

}  // namespace griddly::cuda
