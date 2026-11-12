#define STB_IMAGE_STATIC
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
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

// Renders are compared "close enough", not bit-exact: the SDL CPU (software) renderer, SDL GPU
// drivers (opengl / opengles2 / vulkan) and the CUDA tiled renderer rasterise and filter slightly
// differently (sub-pixel edge placement after rotation, linear filtering of scaled/rotated sprites).
//
// A rendered pixel matches if some reference pixel within `kNeighbourhood` (1px, i.e. a 3x3 window)
// is within `kChannelTolerance` on every RGBA channel. The image matches if at most
// `kMaxMismatchFraction` of pixels have no such match.
//
// Calibrated on the committed goldens (rendered with the SDL CPU renderer) vs the SDL opengles2
// renders of the same tests: worst GPU case is 0.155% at tolerance 16 and 0.104% at tolerance 32,
// while distinct goldens of the same size differ by >= 0.118% (an extra health bar) to several %.
constexpr int kChannelTolerance = 32;
constexpr int kNeighbourhood = 1;
constexpr double kMaxMismatchFraction = 0.0025;

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

  const int width = static_cast<int>(shape[1]);
  const int height = static_cast<int>(shape[2]);
  auto expected = [&](int x, int y, int c) -> int { return *(arg + y * strides[1] * shape[1] + x * strides[1] + c); };
  auto actual = [&](int x, int y, int c) -> int { return *(imageData + y * strides[2] + x * strides[1] + c * strides[0]); };

  int64_t mismatched = 0;
  int worstX = -1, worstY = -1, worstDiff = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int bestDiff = 256;
      for (int dy = -kNeighbourhood; dy <= kNeighbourhood && bestDiff > kChannelTolerance; dy++) {
        for (int dx = -kNeighbourhood; dx <= kNeighbourhood && bestDiff > kChannelTolerance; dx++) {
          const int ex = x + dx, ey = y + dy;
          if (ex < 0 || ey < 0 || ex >= width || ey >= height) continue;
          int d = 0;
          for (int c = 0; c < 4; c++) d = std::max(d, std::abs(actual(x, y, c) - expected(ex, ey, c)));
          bestDiff = std::min(bestDiff, d);
        }
      }
      if (bestDiff > kChannelTolerance) {
        mismatched++;
        if (bestDiff > worstDiff) {
          worstDiff = bestDiff;
          worstX = x;
          worstY = y;
        }
      }
    }
  }

  const double fraction = static_cast<double>(mismatched) / (static_cast<double>(width) * height);
  if (fraction > kMaxMismatchFraction) {
    *result_listener << mismatched << " of " << (width * height) << " pixels (" << fraction * 100.0
                     << "%) differ by more than " << kChannelTolerance << " from the golden image (allowed "
                     << kMaxMismatchFraction * 100.0 << "%); worst at (" << worstX << ", " << worstY
                     << ") by " << worstDiff;
    return false;
  }
  return true;
}

}  // namespace griddly
