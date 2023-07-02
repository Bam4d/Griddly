#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include <stdexcept>

namespace griddly {

std::unique_ptr<uint8_t[]> loadExpectedImage(std::string filename) {
  int width, height, channels;

  stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image.");
  }

  std::unique_ptr<uint8_t[]> spriteData(pixels);

  return std::move(spriteData);
}

int write_image(std::string filename, uint8_t* imageData, int stride, int width, int height) {
  return stbi_write_png(filename.c_str(), width, height, STBI_rgb_alpha, imageData, stride);
}

}  // namespace griddly