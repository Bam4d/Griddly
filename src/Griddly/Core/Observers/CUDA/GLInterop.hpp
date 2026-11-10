#pragma once
#include <cstddef>
#include <cstdint>

// Shares an OpenGL texture with CUDA so the SDL GPU renderer can hand its
// frame to torch without a round trip through host memory. Host-only
// interface; the CUDA runtime stays inside GLInterop.cu.
namespace griddly::cuda {

class GLTextureShare;

// Pixel rect of the texture (texture coordinates, row 0 = top)
struct GLRect {
  uint32_t x, y, w, h;
};

// Registers `glTexture` (GL_TEXTURE_2D, RGBA8, width x height) for read-only
// CUDA access on the CUDA device that drives the current GL context. Throws
// std::runtime_error when the context is not on a CUDA device (e.g. Mesa).
// Must be called with the GL context current.
GLTextureShare* createGLTextureShare(unsigned int glTexture, uint32_t width, uint32_t height);
void destroyGLTextureShare(GLTextureShare* share);

// Copies the texture into a linear device buffer (tightly packed RGBA8, top
// row first) and waits for the copy. The GL context must be current and the
// GL commands writing the texture flushed; mapping the resource orders the
// copy after them (no glFinish needed). Returns the buffer,
// valid until the next copy or destroy.
const uint8_t* copyGLTexture(GLTextureShare* share);
// Same, but copies only the given rects into the buffer; the rest of the
// buffer keeps what the previous copies wrote. One kernel launch for all rects.
const uint8_t* copyGLTextureRects(GLTextureShare* share, const GLRect* rects, size_t count);
int glTextureShareDevice(const GLTextureShare* share);

}  // namespace griddly::cuda
