#include "TiledCUDARenderer.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <stdexcept>

#include "../../GDY/GDYFactory.hpp"
#include "../../Grid.hpp"
#include "../SpriteObserver.hpp"
#include "TiledCUDAKernel.hpp"

namespace griddly {

namespace {

constexpr uint32_t kDefaultLayers = 4;
constexpr uint32_t kMaxNames = 256;

uint32_t quarterTurns(Direction direction) {
  switch (direction) {
    case Direction::RIGHT:
      return 1;
    case Direction::DOWN:
      return 2;
    case Direction::LEFT:
      return 3;
    default:
      return 0;
  }
}

std::shared_ptr<SpriteObserver> makeSpriteObserver(const std::shared_ptr<GDYFactory>& gdyFactory, const std::shared_ptr<Grid>& grid,
                                                   const std::string& observerName) {
  return std::dynamic_pointer_cast<SpriteObserver>(gdyFactory->createObserver(grid, observerName, 1, 0));
}

}  // namespace

std::string TiledCUDARenderer::unsupportedReason(const std::shared_ptr<GDYFactory>& gdyFactory, const std::shared_ptr<Grid>& grid,
                                                 const std::string& observerName) {
  if (!tiledCUDAAvailable()) return "no CUDA device available";
  auto observer = gdyFactory->createObserver(grid, observerName, 1, 0);
  if (observer->getObserverType() != ObserverType::SPRITE_2D) return "observer '" + observerName + "' is not a Sprite2D observer";
  const auto& config = std::static_pointer_cast<SpriteObserver>(observer)->getSpriteConfig();

  if (config.tileSize.x != config.tileSize.y) return "non-square tile size";
  if (config.tileSize.x % 4 != 0) return "tile size must be a multiple of 4";
  if (config.highlightPlayers) return "HighlightPlayers is not supported";
  if (config.fogOfWar.enabled || config.camera.enabled || config.cameraFrame.enabled || config.healthBars.enabled) {
    return "FogOfWar / Camera / CameraFrame / HealthBars are not supported";
  }
  if (config.globalObserverAvatarMode != GlobalObserverAvatarMode::NONE) return "global observer avatar modes are not supported";
  if (config.trackAvatar || config.rotateWithAvatar) return "avatar tracking is not supported";
  if (config.gridXOffset != 0 || config.gridYOffset != 0) return "grid offsets are not supported";
  if (config.overrideGridWidth != 0 || config.overrideGridHeight != 0) return "observer grid size overrides are not supported";
  for (const auto& [name, def] : config.spriteDefinitions) {
    if (def.scale != 1.0f || def.offset != glm::vec2(0, 0)) return "sprite '" + name + "' uses Scale/offset";
    if (def.tilingMode == TilingMode::ISO_FLOOR) return "sprite '" + name + "' uses ISO_FLOOR tiling";
  }
  return "";
}

TiledCUDARenderer::TiledCUDARenderer(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs,
                                     const std::vector<std::shared_ptr<Grid>>& grids,
                                     TiledRendererConfig config)
    : gdyFactory_(std::move(gdyFactory)), config_(std::move(config)), grids_(grids) {
  if (numEnvs == 0) throw std::invalid_argument("TiledCUDARenderer requires >= 1 env");
  if (grids_.size() != numEnvs) throw std::invalid_argument("grids.size() must equal numEnvs");
  numEnvs_ = numEnvs;

  auto reason = unsupportedReason(gdyFactory_, grids_[0], config_.observerName);
  if (!reason.empty()) throw std::invalid_argument("CUDA tiled renderer: " + reason);

  auto observer = makeSpriteObserver(gdyFactory_, grids_[0], config_.observerName);
  const auto& spriteConfig = observer->getSpriteConfig();
  tileSize_ = static_cast<uint32_t>(spriteConfig.tileSize.x);
  rotateSprites_ = spriteConfig.rotateAvatarImage;

  gridWidth_ = grids_[0]->getWidth();
  gridHeight_ = grids_[0]->getHeight();
  for (const auto& g : grids_) {
    if (g->getWidth() != gridWidth_ || g->getHeight() != gridHeight_) {
      throw std::invalid_argument("CUDA tiled renderer requires all envs to have the same grid size");
    }
  }

  computeLayout(numEnvs_, config_, gridWidth_ * tileSize_, gridHeight_ * tileSize_);
  buildAtlas(spriteConfig.resourceConfig.imagePath, tileSize_);
  createKernel(kDefaultLayers);

  spdlog::info("TiledCUDARenderer: device {} ({}x{} envs, frame {}x{}, {} sprites)", config_.cudaDevice, cols_, rows_,
               pixelWidth_, pixelHeight_, atlas_.size() / (tileSize_ * tileSize_));
}

TiledCUDARenderer::~TiledCUDARenderer() {
  cuda::destroyKernel(kernel_);
}

void TiledCUDARenderer::buildAtlas(const std::string& imagePath, uint32_t tileSize) {
  auto observer = makeSpriteObserver(gdyFactory_, grids_[0], config_.observerName);
  glm::ivec2 tile(static_cast<int>(tileSize));
  auto appendImage = [&](const std::string& image) {
    auto pixels = SDLObserver::loadSpriteImage(imagePath + "/" + image, tile);
    size_t offset = atlas_.size();
    atlas_.resize(offset + static_cast<size_t>(tileSize) * tileSize);
    std::memcpy(atlas_.data() + offset, pixels.data(), pixels.size());
  };

  for (const auto& [name, def] : observer->getSpriteConfig().spriteDefinitions) {
    if (def.images.empty()) continue;
    auto atlasBase = static_cast<int16_t>(atlas_.size() / (static_cast<size_t>(tileSize) * tileSize));
    if (name == "_background_") {
      backgroundSprite_ = atlasBase;
      appendImage(def.images[0]);
      continue;
    }
    if (name == "_padding_") continue;  // padding is only drawn outside the grid, which this backend never shows

    int16_t tiling = cuda::TILING_NONE;
    size_t imageCount = 1;
    if (def.tilingMode == TilingMode::WALL_2 || def.tilingMode == TilingMode::WALL_16) {
      tiling = def.tilingMode == TilingMode::WALL_2 ? cuda::TILING_WALL_2 : cuda::TILING_WALL_16;
      imageCount = def.tilingMode == TilingMode::WALL_2 ? 2 : 16;
      if (def.images.size() != imageCount) {
        throw std::invalid_argument(fmt::format("For Tiling Mode WALL_2 and WALL_16, 2 or 16 images must be supplied respectively. {0} images were supplied", def.images.size()));
      }
    }
    defIndex_[name] = static_cast<int>(defTiling_.size());
    defTiling_.push_back(tiling);
    defBase_.push_back(atlasBase);
    for (size_t i = 0; i < imageCount; i++) appendImage(def.images[i]);
  }

  for (const auto& [name, def] : gdyFactory_->getObjectGenerator()->getObjectDefinitions()) {
    if (nameIds_.size() >= kMaxNames) throw std::invalid_argument("CUDA tiled renderer supports at most 256 object types");
    nameIds_.emplace(name, static_cast<uint32_t>(nameIds_.size()));
  }
}

void TiledCUDARenderer::createKernel(uint32_t layers) {
  cuda::TiledKernelDesc desc{};
  desc.numEnvs = static_cast<int>(numEnvs_);
  desc.gridW = static_cast<int>(gridWidth_);
  desc.gridH = static_cast<int>(gridHeight_);
  desc.tileSize = static_cast<int>(tileSize_);
  desc.layers = static_cast<int>(layers);
  desc.numSprites = static_cast<int>(atlas_.size() / (static_cast<size_t>(tileSize_) * tileSize_));
  desc.atlas = atlas_.data();
  desc.numDefs = static_cast<int>(defTiling_.size());
  desc.defTiling = defTiling_.data();
  desc.defBase = defBase_.data();
  desc.backgroundSprite = backgroundSprite_;
  desc.clearColor = clearColor_;
  desc.cols = static_cast<int>(cols_);
  desc.rows = static_cast<int>(rows_);
  desc.gap = static_cast<int>(gap_);
  desc.gapColor = kTiledGapColor;
  desc.device = config_.cudaDevice;

  cuda::TiledKernel* kernel = cuda::createKernel(desc);
  cuda::destroyKernel(kernel_);
  kernel_ = kernel;
  layers_ = layers;
}

uint32_t TiledCUDARenderer::fillEnv(uint32_t envIdx) {
  const size_t cellsPerEnv = static_cast<size_t>(gridWidth_) * gridHeight_ * layers_;
  uint32_t* cells = cuda::hostCells(kernel_) + envIdx * cellsPerEnv;
  std::memset(cells, 0, cellsPerEnv * sizeof(uint32_t));

  thread_local std::vector<int32_t> zs;
  zs.assign(cellsPerEnv, 0);

  auto& grid = *grids_[envIdx];
  uint32_t deepest = 0;
  for (const auto& object : grid.getObjects()) {
    const auto& location = object->getLocation();
    if (location.x < 0 || location.y < 0 || location.x >= static_cast<int32_t>(gridWidth_) || location.y >= static_cast<int32_t>(gridHeight_)) {
      continue;
    }
    const auto& tileName = object->getObjectRenderTileName();
    auto defIt = defIndex_.find(tileName);
    if (defIt == defIndex_.end()) {
      throw std::invalid_argument(fmt::format("Could not find tile definition '{0}' for object '{1}'", tileName, object->getObjectName()));
    }
    const int def = defIt->second;
    uint32_t rotation = 0;
    if (rotateSprites_ && defTiling_[static_cast<size_t>(def)] == cuda::TILING_NONE) {
      rotation = quarterTurns(object->getObjectOrientation().getDirection());
    }
    const uint32_t code = static_cast<uint32_t>(def + 1) | (nameIds_.at(object->getObjectName()) << 12) | (rotation << 20);
    const int32_t z = object->getZIdx();

    // insertion sort by z, stable on getObjects() order like SDLObserver::drawItems
    const size_t base = (static_cast<size_t>(location.y) * gridWidth_ + static_cast<size_t>(location.x)) * layers_;
    uint32_t n = 0;
    while (n < layers_ && cells[base + n] != 0) n++;
    if (n == layers_) {
      deepest = layers_ + 1;  // caller grows the layer count and refills
      continue;
    }
    uint32_t k = n;
    while (k > 0 && zs[base + k - 1] > z) {
      cells[base + k] = cells[base + k - 1];
      zs[base + k] = zs[base + k - 1];
      k--;
    }
    cells[base + k] = code;
    zs[base + k] = z;
    deepest = std::max(deepest, n + 1);
  }
  grid.purgeUpdatedLocations(0);
  return deepest;
}

uint32_t TiledCUDARenderer::fillCells(bool parallel, uint32_t numThreads) {
  std::atomic<uint32_t> deepest{0};
  // filling one env takes microseconds, so give each thread a batch of envs
  parallelFor(numEnvs_, parallel ? numThreads : 1, 8, [&](uint32_t i) {
    uint32_t d = fillEnv(i);
    uint32_t prev = deepest.load();
    while (prev < d && !deepest.compare_exchange_weak(prev, d)) {
    }
  });
  return deepest.load();
}

void TiledCUDARenderer::prepareFrame(bool parallel, uint32_t numThreads) {
  // Make sure the previous frame's upload of the pinned cell buffer is done before overwriting it
  cuda::deviceOutput(kernel_);
  if (fillCells(parallel, numThreads) > layers_) {
    // Some cell stacks more objects than we have layers: grow until everything fits
    uint32_t layers = layers_;
    do {
      createKernel(layers * 2);
      layers = layers_;
    } while (fillCells(parallel, numThreads) > layers_);
  }
  cuda::renderAsync(kernel_);
}

const uint8_t* TiledCUDARenderer::render(bool parallel, uint32_t numThreads) {
  prepareFrame(parallel, numThreads);
  return cuda::readback(kernel_);
}

const uint8_t* TiledCUDARenderer::renderDevice(bool parallel, uint32_t numThreads) {
  prepareFrame(parallel, numThreads);
  return cuda::deviceOutput(kernel_);
}

}  // namespace griddly
