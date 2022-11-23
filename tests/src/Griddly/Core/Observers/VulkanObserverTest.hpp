#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <memory>

using ::testing::Return;

namespace griddly {

inline std::unique_ptr<uint8_t[]> loadExpectedImage(std::string filename) {
  int width, height, channels;

  stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image.");
  }

  auto spriteSize = width * height * channels;

  std::unique_ptr<uint8_t[]> spriteData(pixels);

  return std::move(spriteData);
}

inline int write_image(std::string filename, uint8_t* imageData, int stride, int width, int height) {
  return stbi_write_png(filename.c_str(), width, height, STBI_rgb_alpha, imageData, stride);
}

MATCHER_P3(ObservationResultMatcher, shape, strides, imageData, "") {
  for (int x = 0; x < shape[1]; x++) {
    for (int y = 0; y < shape[2]; y++) {
      for (int c = 0; c < 4; c++) {
        uint8_t srcImageBit = *(arg + y * strides[1] * shape[1] + x * strides[1] + c);
        uint8_t dstImageBit = *(imageData + y * strides[2] + x * strides[1] + c * strides[0]);

        // Tolerate error of 1 bit in the src and dst images as rendering
        // deviates a little on different devices.
        if (!(dstImageBit - 1 <= srcImageBit && srcImageBit <= dstImageBit + 1)) {
          spdlog::debug("source x: {0} y: {1} c: {2} ({3} != {4})",
                        x, y, c, srcImageBit, dstImageBit);
          return false;
        }
      }
    }
  }
  return true;
}

}  // namespace griddly
