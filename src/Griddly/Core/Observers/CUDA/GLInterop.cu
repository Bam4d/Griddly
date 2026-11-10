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
    if (stream_ != nullptr) cudaStreamDestroy(stream_);
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
};

GLTextureShare* createGLTextureShare(unsigned int glTexture, uint32_t width, uint32_t height) {
  return new GLTextureShare(glTexture, width, height);
}
void destroyGLTextureShare(GLTextureShare* share) { delete share; }
const uint8_t* copyGLTexture(GLTextureShare* share) { return share->copy(); }
int glTextureShareDevice(const GLTextureShare* share) { return share->device(); }

}  // namespace griddly::cuda
