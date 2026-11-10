// Parity tests for the tiled renderers.
//
// Reference: TiledSDLRenderer on the SDL software renderer (SDL CPU), pinned
// by a golden image so the reference itself cannot drift. The other backends
// are compared against it frame by frame while the envs step:
//   - CUDA (TiledCUDARenderer) must be pixel-identical
//   - SDL GPU (TiledSDLRenderer on a GL driver) must be within 1 LSB per
//     channel, GL blends in floating point
//   - the tiled SDL CPU output must equal each env rendered on its own by a
//     plain SDL observer
// Backends that are not available (no CUDA device, no GPU SDL driver) skip,
// or fail when listed in GRIDDLY_REQUIRE_TILED_BACKENDS.
// Regenerate the golden image with GRIDDLY_UPDATE_GOLDEN_IMAGES=1.
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/Observers/TiledRenderer.hpp"
#include "Griddly/Core/Observers/TiledSDLRenderer.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"
#include "ObserverTestUtils.hpp"  // stb image read/write
#ifdef GRIDDLY_HAS_CUDA
#include "Griddly/Core/Observers/CUDA/TiledCUDAKernel.hpp"
#endif

namespace griddly {

namespace {

const std::string kFixture = "tests/resources/tiled_parity.yaml";
const std::string kGolden = "tests/resources/observer/tiled/tiled_parity_sprite2d_level0.png";

std::shared_ptr<GDYFactory> loadGame(const std::string& gdyPath) {
  ResourceConfig resourceConfig{"resources/games", "resources/images", ""};
  auto gdyFactory = std::make_shared<GDYFactory>(std::make_shared<ObjectGenerator>(), std::make_shared<TerminationGenerator>(), resourceConfig);
  gdyFactory->initializeFromFile(gdyPath);
  return gdyFactory;
}

std::vector<std::shared_ptr<Grid>> envGrids(const VectorizedEnv& env) {
  std::vector<std::shared_ptr<Grid>> grids;
  for (uint32_t i = 0; i < env.numEnvs(); i++) grids.push_back(env.grid(i));
  return grids;
}

struct ImageDiff {
  size_t pixels = 0;  // pixels with any differing channel
  int maxChannel = 0;
  int firstX = -1, firstY = -1;
};

ImageDiff diffImages(const uint8_t* a, const uint8_t* b, uint32_t width, uint32_t height, uint32_t pitchA, uint32_t pitchB) {
  ImageDiff d;
  for (uint32_t y = 0; y < height; y++) {
    const uint8_t* ra = a + static_cast<size_t>(y) * pitchA;
    const uint8_t* rb = b + static_cast<size_t>(y) * pitchB;
    for (uint32_t x = 0; x < width; x++) {
      int pixelMax = 0;
      for (int c = 0; c < 4; c++) pixelMax = std::max(pixelMax, std::abs(static_cast<int>(ra[x * 4 + c]) - static_cast<int>(rb[x * 4 + c])));
      if (pixelMax > 0) {
        if (d.pixels == 0) {
          d.firstX = static_cast<int>(x);
          d.firstY = static_cast<int>(y);
        }
        d.pixels++;
        d.maxChannel = std::max(d.maxChannel, pixelMax);
      }
    }
  }
  return d;
}

ImageDiff diffFrames(const TiledRenderer& r, const uint8_t* a, const uint8_t* b) {
  return diffImages(a, b, r.width(), r.height(), r.width() * 4, r.width() * 4);
}

std::string describe(const ImageDiff& d) {
  return std::to_string(d.pixels) + " pixels differ, max channel diff " + std::to_string(d.maxChannel) +
         ", first at (" + std::to_string(d.firstX) + ", " + std::to_string(d.firstY) + ")";
}

// Sets an environment variable for the lifetime of the object, restoring the previous value
class ScopedEnv {
 public:
  ScopedEnv(const char* name, const char* value) : name_(name) {
    const char* previous = std::getenv(name);
    hadPrevious_ = previous != nullptr;
    if (hadPrevious_) previous_ = previous;
    if (value != nullptr) setenv(name, value, 1);
    else unsetenv(name);
  }
  ~ScopedEnv() {
    if (hadPrevious_) setenv(name_.c_str(), previous_.c_str(), 1);
    else unsetenv(name_.c_str());
  }
  ScopedEnv(const ScopedEnv&) = delete;
  ScopedEnv& operator=(const ScopedEnv&) = delete;

 private:
  std::string name_;
  bool hadPrevious_ = false;
  std::string previous_;
};

// Skips a test when a backend is missing, unless GRIDDLY_REQUIRE_TILED_BACKENDS
// lists it (comma separated: "cuda", "sdl_gpu"); GPU CI runners set it so a
// broken driver or build cannot silently turn the parity checks into skips.
bool backendRequired(const std::string& backend) {
  const char* required = std::getenv("GRIDDLY_REQUIRE_TILED_BACKENDS");
  if (required == nullptr) return false;
  std::string list = std::string(",") + required + ",";
  return list.find("," + backend + ",") != std::string::npos;
}

#define SKIP_OR_FAIL_WITHOUT(backend, reason)                                                  \
  do {                                                                                         \
    if (backendRequired(backend)) FAIL() << backend << " required but unavailable: " << reason; \
    GTEST_SKIP() << reason;                                                                    \
  } while (0)

std::unique_ptr<TiledSDLRenderer> makeSDLCPU(const std::shared_ptr<GDYFactory>& gdy, const VectorizedEnv& env, const std::string& observerName) {
  ScopedEnv forceCpu("GRIDDLY_SDL_FORCE_CPU", "1");
  TiledRendererConfig config;
  config.observerName = observerName;
  auto r = std::make_unique<TiledSDLRenderer>(gdy, env.numEnvs(), envGrids(env), config);
  if (r->isUsingGPU()) throw std::runtime_error("SDL CPU reference unexpectedly on the GPU");
  return r;
}

// nullptr when SDL has no GPU render driver here
std::unique_ptr<TiledSDLRenderer> makeSDLGPU(const std::shared_ptr<GDYFactory>& gdy, const VectorizedEnv& env, const std::string& observerName) {
  ScopedEnv noForce("GRIDDLY_SDL_FORCE_CPU", nullptr);
  TiledRendererConfig config;
  config.observerName = observerName;
  auto r = std::make_unique<TiledSDLRenderer>(gdy, env.numEnvs(), envGrids(env), config);
  if (!r->isUsingGPU()) return nullptr;
  return r;
}

// nullptr when CUDA is unavailable
std::unique_ptr<TiledRenderer> makeCUDA(const std::shared_ptr<GDYFactory>& gdy, const VectorizedEnv& env, const std::string& observerName) {
  if (!tiledCUDAAvailable()) return nullptr;
  TiledRendererConfig config;
  config.observerName = observerName;
  config.backend = TiledRenderBackend::CUDA;
  return createTiledRenderer(gdy, env.numEnvs(), envGrids(env), config);
}

// Deterministic pseudo-random actions for every env
class ActionSource {
 public:
  ActionSource(uint32_t numEnvs, uint32_t numActions) : numEnvs_(numEnvs), numActions_(numActions) {}
  std::vector<std::vector<std::vector<int32_t>>> next() {
    std::vector<std::vector<std::vector<int32_t>>> actions(numEnvs_);
    for (auto& a : actions) {
      state_ = state_ * 1664525u + 1013904223u;
      a.push_back({static_cast<int32_t>(1 + ((state_ >> 16) % numActions_))});
    }
    return actions;
  }

 private:
  uint32_t numEnvs_, numActions_;
  uint32_t state_ = 12345;
};

struct ParityCase {
  std::string gdy;
  uint32_t level;
  std::string observer;
  uint32_t numActions;  // action ids 1..numActions are sent to every env
};

std::string caseName(const ParityCase& c) {
  return c.gdy + " level " + std::to_string(c.level) + " observer " + c.observer;
}

const std::vector<ParityCase>& spriteCases() {
  static const std::vector<ParityCase> cases = {
      {kFixture, 0, "Sprite2D", 3},
      {kFixture, 1, "Sprite2D", 3},
      {kFixture, 0, "Sprite2DNoRotate", 3},
      {"resources/games/Single-Player/Mini-Grid/minigrid-spiders.yaml", 4, "Sprite2D", 4},
      {"resources/games/Single-Player/GVGAI/zelda.yaml", 0, "Sprite2D", 4},
      {"resources/games/Single-Player/GVGAI/sokoban.yaml", 0, "Sprite2D", 4},
      {"resources/games/Single-Player/GVGAI/bait.yaml", 0, "Sprite2D", 4},
  };
  return cases;
}

constexpr uint32_t kSteps = 12;
constexpr int kGPUTolerance = 3;

}  // namespace

// --- Layout and backend selection -------------------------------------------

TEST(TiledRendererTest, LayoutAndGaps) {
  auto gdy = loadGame(kFixture);
  VectorizedEnv env(gdy, 5, 2);
  env.loadLevel(0);
  auto r = makeSDLCPU(gdy, env, "Sprite2D");
  ASSERT_EQ(r->cols(), 3u);
  ASSERT_EQ(r->rows(), 2u);
  EXPECT_EQ(r->width(), 3 * r->envPixelWidth() + 4 * 2);
  EXPECT_EQ(r->height(), 2 * r->envPixelHeight() + 3 * 2);
  EXPECT_EQ(r->envOffsetX(4), 2 + 1 * (r->envPixelWidth() + 2));
  EXPECT_EQ(r->envOffsetY(4), 2 + 1 * (r->envPixelHeight() + 2));

  const auto* px = reinterpret_cast<const uint32_t*>(r->render(true, 2));
  // gaps and the unused sixth slot are the gap colour
  EXPECT_EQ(px[0], kTiledGapColor);
  EXPECT_EQ(px[static_cast<size_t>(r->height() - 1) * r->width() + r->width() - 1], kTiledGapColor);
  const uint32_t unusedX = r->envOffsetX(5) + r->envPixelWidth() / 2;
  const uint32_t unusedY = r->envOffsetY(5) + r->envPixelHeight() / 2;
  EXPECT_EQ(px[static_cast<size_t>(unusedY) * r->width() + unusedX], kTiledGapColor);
  // env views are drawn over
  EXPECT_NE(px[static_cast<size_t>(r->envOffsetY(0) + 1) * r->width() + r->envOffsetX(0) + 1], kTiledGapColor);
}

TEST(TiledRendererTest, ExplicitSDLBackend) {
  ScopedEnv backend("GRIDDLY_TILED_BACKEND", nullptr);
  ScopedEnv forceCpu("GRIDDLY_SDL_FORCE_CPU", "1");
  auto gdy = loadGame(kFixture);
  VectorizedEnv env(gdy, 2, 1);
  env.loadLevel(0);
  TiledRendererConfig config;
  config.backend = TiledRenderBackend::SDL;
  EXPECT_EQ(createTiledRenderer(gdy, 2, envGrids(env), config)->backend(), TiledRenderBackend::SDL);
}

TEST(TiledRendererTest, AutoFallsBackToSDLForUnsupportedObservers) {
  ScopedEnv backend("GRIDDLY_TILED_BACKEND", nullptr);
  ScopedEnv forceCpu("GRIDDLY_SDL_FORCE_CPU", "1");
  auto gdy = loadGame(kFixture);
  VectorizedEnv env(gdy, 2, 1);
  env.loadLevel(0);
  TiledRendererConfig config;
  config.observerName = "Block2D";  // shapes and colours, not supported by the CUDA kernel
  config.backend = TiledRenderBackend::AUTO;
  EXPECT_EQ(createTiledRenderer(gdy, 2, envGrids(env), config)->backend(), TiledRenderBackend::SDL);

  config.backend = TiledRenderBackend::CUDA;
  EXPECT_THROW(createTiledRenderer(gdy, 2, envGrids(env), config), std::invalid_argument);
}

TEST(TiledRendererTest, AutoPicksCUDAWhenSupported) {
  if (!tiledCUDAAvailable()) SKIP_OR_FAIL_WITHOUT("cuda", "CUDA backend not built or no CUDA device");
  ScopedEnv backend("GRIDDLY_TILED_BACKEND", nullptr);
  auto gdy = loadGame(kFixture);
  VectorizedEnv env(gdy, 2, 1);
  env.loadLevel(0);
  TiledRendererConfig config;
  EXPECT_EQ(createTiledRenderer(gdy, 2, envGrids(env), config)->backend(), TiledRenderBackend::CUDA);
  ScopedEnv forceSdl("GRIDDLY_TILED_BACKEND", "sdl");
  EXPECT_EQ(createTiledRenderer(gdy, 2, envGrids(env), config)->backend(), TiledRenderBackend::SDL);
}

// --- Reference ---------------------------------------------------------------

// Pins the SDL CPU reference: tiling, rotation, stacking, background and
// render tile ids on the fixture level, 1 env.
TEST(TiledRendererTest, SDLCPUMatchesGoldenImage) {
  auto gdy = loadGame(kFixture);
  VectorizedEnv env(gdy, 1, 1);
  env.loadLevel(0);
  auto r = makeSDLCPU(gdy, env, "Sprite2D");
  const uint8_t* px = r->render(true, 1);

  if (std::getenv("GRIDDLY_UPDATE_GOLDEN_IMAGES") != nullptr) {
    ASSERT_NE(write_image(kGolden, const_cast<uint8_t*>(px), static_cast<int>(r->width() * 4), static_cast<int>(r->width()), static_cast<int>(r->height())), 0);
    GTEST_SKIP() << "golden image written to " << kGolden;
  }

  int w = 0, h = 0, channels = 0;
  std::unique_ptr<stbi_uc, void (*)(void*)> golden(stbi_load(kGolden.c_str(), &w, &h, &channels, STBI_rgb_alpha), stbi_image_free);
  ASSERT_NE(golden, nullptr) << "missing golden image " << kGolden << ", run with GRIDDLY_UPDATE_GOLDEN_IMAGES=1";
  ASSERT_EQ(static_cast<uint32_t>(w), r->width());
  ASSERT_EQ(static_cast<uint32_t>(h), r->height());
  auto d = diffFrames(*r, px, golden.get());
  // same 1 LSB tolerance as the observer golden-image tests (stb/SDL builds differ across platforms)
  EXPECT_LE(d.maxChannel, 1) << describe(d);
}

// The tiled SDL CPU frame is exactly each env rendered on its own by a plain
// SDL observer, i.e. drawing straight into the composite changes nothing.
TEST(TiledRendererTest, SDLCPUMatchesStandaloneObservers) {
  ScopedEnv forceCpu("GRIDDLY_SDL_FORCE_CPU", "1");
  for (const char* baseLayer : {"cached base layer", "no base layer"}) {
  ScopedEnv noBase("GRIDDLY_SDL_NO_BASE_LAYER", std::string(baseLayer) == "no base layer" ? "1" : nullptr);
  for (const auto& tc : {ParityCase{kFixture, 1, "Sprite2D", 3}, ParityCase{kFixture, 0, "Block2D", 3}}) {
    SCOPED_TRACE(std::string(baseLayer) + ": " + caseName(tc));
    auto gdy = loadGame(tc.gdy);
    const uint32_t numEnvs = 5;
    VectorizedEnv env(gdy, numEnvs, 2);
    env.loadLevel(tc.level);
    env.seed(77);
    auto tiled = makeSDLCPU(gdy, env, tc.observer);

    std::vector<std::shared_ptr<SDLObserver>> standalone;
    for (uint32_t i = 0; i < numEnvs; i++) {
      auto o = std::dynamic_pointer_cast<SDLObserver>(gdy->createObserver(env.grid(i), tc.observer, 1, 0));
      o->init({});
      o->reset();
      standalone.push_back(o);
    }

    ActionSource actions(numEnvs, tc.numActions);
    for (uint32_t s = 0; s < 4; s++) {
      if (s > 0) env.step(actions.next());
      const uint8_t* frame = tiled->render(true, 2);
      for (uint32_t i = 0; i < numEnvs; i++) {
        const uint8_t* own = &standalone[i]->update();
        const uint8_t* rect = frame + (static_cast<size_t>(tiled->envOffsetY(i)) * tiled->width() + tiled->envOffsetX(i)) * 4;
        auto d = diffImages(rect, own, tiled->envPixelWidth(), tiled->envPixelHeight(), tiled->width() * 4, tiled->envPixelWidth() * 4);
        ASSERT_EQ(d.pixels, 0u) << "step " << s << " env " << i << ": " << describe(d);
      }
    }
    for (auto& o : standalone) o->release();
  }
  }
}

// The CPU path only repaints cells whose sprites changed; reloading or
// resetting levels under it must not leave stale pixels behind.
TEST(TiledRendererTest, SDLCPUIncrementalSurvivesResets) {
  ScopedEnv forceCpu("GRIDDLY_SDL_FORCE_CPU", "1");
  auto gdy = loadGame(kFixture);
  const uint32_t numEnvs = 4;
  VectorizedEnv env(gdy, numEnvs, 2);
  env.loadLevel(1);
  env.seed(5);
  auto incremental = makeSDLCPU(gdy, env, "Sprite2D");
  std::unique_ptr<TiledSDLRenderer> full;
  {
    ScopedEnv noDirty("GRIDDLY_SDL_NO_DIRTY_CELLS", "1");
    full = makeSDLCPU(gdy, env, "Sprite2D");
  }
  ActionSource actions(numEnvs, 3);
  std::vector<uint8_t> expected;
  auto check = [&](const std::string& when) {
    ScopedEnv noDirty("GRIDDLY_SDL_NO_DIRTY_CELLS", "1");
    const uint8_t* f = full->render(true, 2);
    expected.assign(f, f + static_cast<size_t>(full->width()) * full->height() * 4);
    unsetenv("GRIDDLY_SDL_NO_DIRTY_CELLS");
    auto d = diffFrames(*incremental, incremental->render(true, 2), expected.data());
    ASSERT_EQ(d.pixels, 0u) << when << ": " << describe(d);
  };
  for (uint32_t s = 0; s < 6; s++) {
    env.step(actions.next());
    check("step " + std::to_string(s));
  }
  env.reset();
  check("after reset");
  for (uint32_t s = 0; s < 3; s++) {
    env.step(actions.next());
    check("step after reset " + std::to_string(s));
  }
}

// --- Cross-backend parity ------------------------------------------------------

TEST(TiledRendererTest, CUDAMatchesSDLCPU) {
  if (!tiledCUDAAvailable()) SKIP_OR_FAIL_WITHOUT("cuda", "CUDA backend not built or no CUDA device");
  for (const auto& tc : spriteCases()) {
    for (uint32_t numEnvs : {1u, 7u, 16u}) {
      SCOPED_TRACE(caseName(tc) + " envs " + std::to_string(numEnvs));
      auto gdy = loadGame(tc.gdy);
      VectorizedEnv env(gdy, numEnvs, 4);
      env.loadLevel(tc.level);
      env.seed(1234);
      auto reference = makeSDLCPU(gdy, env, tc.observer);
      auto cuda = makeCUDA(gdy, env, tc.observer);
      ASSERT_EQ(cuda->backend(), TiledRenderBackend::CUDA);
      ASSERT_EQ(cuda->width(), reference->width());
      ASSERT_EQ(cuda->height(), reference->height());

      ActionSource actions(numEnvs, tc.numActions);
      std::vector<uint8_t> expected;
      for (uint32_t s = 0; s <= kSteps; s++) {
        if (s > 0) env.step(actions.next());
        const uint8_t* ref = reference->render(true, 4);
        expected.assign(ref, ref + static_cast<size_t>(reference->width()) * reference->height() * 4);
        auto d = diffFrames(*cuda, cuda->render(true, 4), expected.data());
        ASSERT_EQ(d.pixels, 0u) << "step " << s << ": " << describe(d);
      }
    }
  }
}

// The SDL GPU path draws cell-aligned scenes as batched SDL_RenderGeometry
// calls and everything else per env; GRIDDLY_SDL_NO_BATCH and
// GRIDDLY_SDL_NO_BASE_LAYER force the slower variants so all of them are
// checked against the CPU reference.
TEST(TiledRendererTest, SDLGPUMatchesSDLCPU) {
  bool ranAny = false;
  const std::vector<std::pair<const char*, const char*>> variants = {
      {"batched", nullptr}, {"per-env draw", "GRIDDLY_SDL_NO_BATCH"}, {"no base layer", "GRIDDLY_SDL_NO_BASE_LAYER"}};
  for (const auto& variant : variants) {
  ScopedEnv noBatch("GRIDDLY_SDL_NO_BATCH", variant.second != nullptr && std::string(variant.second) == "GRIDDLY_SDL_NO_BATCH" ? "1" : nullptr);
  ScopedEnv noBase("GRIDDLY_SDL_NO_BASE_LAYER", variant.second != nullptr && std::string(variant.second) == "GRIDDLY_SDL_NO_BASE_LAYER" ? "1" : nullptr);
  for (const auto& tc : {ParityCase{kFixture, 0, "Sprite2D", 3}, ParityCase{kFixture, 1, "Sprite2D", 3},
                         ParityCase{kFixture, 0, "Sprite2DNoRotate", 3}, ParityCase{kFixture, 1, "Block2D", 3},
                         ParityCase{"resources/games/Single-Player/Mini-Grid/minigrid-spiders.yaml", 4, "Sprite2D", 4},
                         ParityCase{"resources/games/Single-Player/GVGAI/zelda.yaml", 0, "Sprite2D", 4}}) {
    SCOPED_TRACE(std::string(variant.first) + ": " + caseName(tc));
    auto gdy = loadGame(tc.gdy);
    const uint32_t numEnvs = 7;
    VectorizedEnv env(gdy, numEnvs, 4);
    env.loadLevel(tc.level);
    env.seed(4321);
    auto gpu = makeSDLGPU(gdy, env, tc.observer);
    if (gpu == nullptr) SKIP_OR_FAIL_WITHOUT("sdl_gpu", "no SDL GPU render driver available");
    ranAny = true;
    auto reference = makeSDLCPU(gdy, env, tc.observer);

    ActionSource actions(numEnvs, tc.numActions);
    std::vector<uint8_t> gpuFrame;
    for (uint32_t s = 0; s <= kSteps; s++) {
      if (s > 0) env.step(actions.next());
      const uint8_t* g = gpu->render(true, 4);
      gpuFrame.assign(g, g + static_cast<size_t>(gpu->width()) * gpu->height() * 4);
      auto d = diffFrames(*gpu, gpuFrame.data(), reference->render(true, 4));
      // GL blends in floating point, SDL's software blitters with integer
      // shifts: up to 1 LSB per translucent layer, so a deep stack of
      // translucent sprites can drift by a few. Anything missing, misplaced or
      // mis-rotated differs by far more.
      ASSERT_LE(d.maxChannel, kGPUTolerance) << "step " << s << ": " << describe(d);
    }
  }
  }
  EXPECT_TRUE(ranAny);
}

// --- CUDA-specific paths -----------------------------------------------------

// renderDevice() must hold the same frame as render(), and switching between
// them must not lose updates.
TEST(TiledRendererTest, CUDADeviceOutputMatchesHost) {
#ifndef GRIDDLY_HAS_CUDA
  SKIP_OR_FAIL_WITHOUT("cuda", "CUDA backend not built");
#else
  if (!tiledCUDAAvailable()) SKIP_OR_FAIL_WITHOUT("cuda", "no CUDA device");
  auto gdy = loadGame(kFixture);
  const uint32_t numEnvs = 6;
  VectorizedEnv env(gdy, numEnvs, 2);
  env.loadLevel(1);
  env.seed(9);
  auto cuda = makeCUDA(gdy, env, "Sprite2D");
  const size_t bytes = static_cast<size_t>(cuda->width()) * cuda->height() * 4;
  ActionSource actions(numEnvs, 3);
  std::vector<uint8_t> host, device(bytes);
  for (uint32_t s = 0; s < 4; s++) {
    env.step(actions.next());
    const uint8_t* devicePtr = cuda->renderDevice(true, 2);
    ASSERT_NE(devicePtr, nullptr);
    cuda::copyDeviceToHost(device.data(), devicePtr, bytes);
    const uint8_t* h = cuda->render(true, 2);
    host.assign(h, h + bytes);
    ASSERT_EQ(host, device) << "step " << s;
  }
#endif
}

// The fixture stacks 5 objects in one cell; the renderer starts with 4 layers
// per cell and must grow without dropping the top object.
TEST(TiledRendererTest, CUDAGrowsLayersForDeepStacks) {
  if (!tiledCUDAAvailable()) SKIP_OR_FAIL_WITHOUT("cuda", "CUDA backend not built or no CUDA device");
  auto gdy = loadGame(kFixture);
  VectorizedEnv env(gdy, 3, 1);
  env.loadLevel(0);
  auto reference = makeSDLCPU(gdy, env, "Sprite2D");
  std::vector<uint8_t> expected;
  const uint8_t* ref = reference->render(true, 1);
  expected.assign(ref, ref + static_cast<size_t>(reference->width()) * reference->height() * 4);
  auto cuda = makeCUDA(gdy, env, "Sprite2D");
  auto d = diffFrames(*cuda, cuda->render(true, 1), expected.data());
  EXPECT_EQ(d.pixels, 0u) << describe(d);
}

}  // namespace griddly
