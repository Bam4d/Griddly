#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>

using ::testing::Return;

namespace griddly {

// Golden images are per-test (`<name>.<TestName>.png`) when present, so tests
// that used to share one expected image under Vulkan can have slightly
// different renders under SDL. Falls back to the shared file otherwise.
inline std::string perTestGoldenPath(const std::string& filename) {
  const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
  if (testInfo == nullptr) {
    return filename;
  }
  std::string perTest = filename;
  auto pos = perTest.rfind(".png");
  if (pos != std::string::npos) {
    perTest.insert(pos, "." + std::string(testInfo->name()));
  } else {
    perTest += "." + std::string(testInfo->name());
  }
  return perTest;
}

// Paths the matcher should rewrite when GRIDDLY_UPDATE_GOLDEN_IMAGES=1
inline std::string& goldenWritePath() {
  static std::string filename;
  return filename;
}

inline std::string& goldenSharedWritePath() {
  static std::string filename;
  return filename;
}

inline std::unique_ptr<uint8_t[]> loadExpectedImage(std::string filename) {
  std::string perTest = perTestGoldenPath(filename);
  if (std::filesystem::exists(perTest)) {
    filename = perTest;
  }
  goldenWritePath() = perTest;        // in update mode the matcher writes here
  goldenSharedWritePath() = filename; // and here if it resolved to the shared file


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
  if (std::getenv("GRIDDLY_UPDATE_GOLDEN_IMAGES") != nullptr) {
    // Regenerate the golden image from the current renderer output
    // NB: in this matcher `arg` is the expected image and `imageData` is the actual render
    write_image(goldenWritePath(), const_cast<uint8_t*>(imageData), strides[1] * shape[1], shape[1], shape[2]);
    if (goldenSharedWritePath() != goldenWritePath()) {
      write_image(goldenSharedWritePath(), const_cast<uint8_t*>(imageData), strides[1] * shape[1], shape[1], shape[2]);
    }
    return true;
  }

  for (int x = 0; x < shape[1]; x++) {
    for (int y = 0; y < shape[2]; y++) {
      for (int c = 0; c < 4; c++) {
        uint8_t srcImageBit = *(arg + y * strides[1] * shape[1] + x * strides[1] + c);
        uint8_t dstImageBit = *(imageData + y * strides[2] + x * strides[1] + c * strides[0]);

        // Tolerate error of 1 bit in the src and dst images as rendering
        // deviates a little on different devices.
        if (!(dstImageBit - 1 <= srcImageBit && srcImageBit <= dstImageBit + 1)) {
          return false;
        }
      }
    }
  }
  return true;
}

}  // namespace griddly
