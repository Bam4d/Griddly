#include "SDLObserver.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#include <glm/gtx/color_space.hpp>

// Image loading is done here so the implementation is compiled exactly once
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

namespace griddly {

// Video subsystem is shared between all observers, so reference count it
static int sdlVideoRefCount = 0;

// When set, observers create a visible window instead of a hidden one
static bool sdlWindowVisible = false;

void setSDLWindowVisible(bool visible) {
  sdlWindowVisible = visible;
}

SDLObserver::SDLObserver(std::shared_ptr<Grid> grid, SDLObserverConfig& config) : Observer(std::move(grid), config) {
  config_ = config;
}

SDLObserver::~SDLObserver() {
  release();
}

void SDLObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  Observer::init(playerObservers);

  uint32_t playerCount = grid_->getPlayerCount();
  if (config_.playerColors.size() > 0) {
    if (config_.playerColors.size() >= playerCount) {
      for (const auto& playerColor : config_.playerColors) {
        playerColors_.emplace_back(glm::vec4(playerColor, 1.0));
      }
    } else {
      std::string error = fmt::format("The number of player colors is less than the number of players defined.");
      spdlog::error(error);
      throw std::invalid_argument(error);
    }
  } else {
    float s = 1.0F;
    float v = 0.6F;
    float h_inc = 360.0F / playerCount;
    for (uint32_t p = 0; p < playerCount; p++) {
      uint32_t h = h_inc * p;
      glm::vec4 rgba = glm::vec4(glm::rgbColor(glm::vec3(h, s, v)), 1.0);
      playerColors_.push_back(rgba);
    }
  }
}

void SDLObserver::initSDL() {
  // Decide between a GPU renderer (hidden window, any non-software driver SDL has)
  // and the pure-CPU software renderer that needs no display at all.
  const char* driver = nullptr;
#ifdef __EMSCRIPTEN__
  // In the browser, keep SDL's window on its own (hidden) canvas so the app's
  // visible 2D canvas (#canvas) stays free for blitting the rendered pixels
  SDL_SetHint(SDL_HINT_EMSCRIPTEN_CANVAS_SELECTOR, "#sdl-offscreen");
#endif
  // Collect all non-software render drivers, in order; we try each until one works.
  std::vector<std::string> gpuDrivers;
  // An external pixel target means the caller wants the software renderer
  if (std::getenv("GRIDDLY_SDL_FORCE_CPU") == nullptr && externalPixels_ == nullptr) {
    // Headless: the SDL_HINT_VIDEO_DRIVER hint (set BEFORE our SDL_Init) makes
    // SDL pick the offscreen video driver, which registers all render drivers
    // incl. opengles2 and allows hidden OPENGL windows on the GPU. Setting the
    // env var here is too late - SDL reads it only inside SDL_Init.
    if (std::getenv("DISPLAY") == nullptr && std::getenv("WAYLAND_DISPLAY") == nullptr &&
        std::getenv("SDL_VIDEODRIVER") == nullptr) {
      SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
    }
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
      // Use full SDL_Init (not SDL_InitSubSystem): with the video-driver hint
      // set above, SDL_Init(SDL_INIT_VIDEO) registers all render drivers
      // (opengles2/vulkan/gpu/software). SDL_InitSubSystem alone leaves SDL in
      // a state where only the software driver is visible.
      if (SDL_Init(SDL_INIT_VIDEO)) {
        videoInitialized_ = true;
        sdlVideoRefCount++;
      } else {
        spdlog::info("SDL: video subsystem unavailable ({}), probing render drivers headless", SDL_GetError());
      }
    } else {
      videoInitialized_ = false;  // someone else owns the video subsystem
    }
    const char* forced = std::getenv("GRIDDLY_SDL_DRIVER");
    if (forced != nullptr) {
      gpuDrivers.emplace_back(forced);
    } else {
      int driverCount = SDL_GetNumRenderDrivers();
      for (int i = 0; i < driverCount; i++) {
        const char* name = SDL_GetRenderDriver(i);
        if (name == nullptr || SDL_strcasecmp(name, "software") == 0) continue;
        // Prefer EGL (opengles2): works headless on datacenter GPUs via
        // EGL surfaceless, more portable than Vulkan there.
        if (SDL_strcasecmp(name, "opengles2") == 0) gpuDrivers.insert(gpuDrivers.begin(), name);
        else gpuDrivers.emplace_back(name);
      }
    }
    if (gpuDrivers.empty()) spdlog::info("SDL: no GPU render drivers available, CPU software renderer");
  } else {
    spdlog::info("SDL: GRIDDLY_SDL_FORCE_CPU set, using CPU software renderer");
  }

  for (const auto& drv : gpuDrivers) {
    SDL_WindowFlags flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL;
#ifdef __EMSCRIPTEN__
    // In the browser we always render offscreen and blit pixels to a 2D canvas
    // ourselves; a visible SDL window's event handlers are unreliable in the port
#else
    if (config_.renderToCanvas || sdlWindowVisible || std::getenv("GRIDDLY_SDL_WINDOW") != nullptr) {
      flags = 0;  // visible window for human viewing
    }
#endif
    window_ = SDL_CreateWindow("Griddly", static_cast<int>(pixelWidth_), static_cast<int>(pixelHeight_), flags);
    if (window_ != nullptr) {
      renderer_ = SDL_CreateRenderer(window_, drv.c_str());
      if (renderer_ == nullptr) {
        spdlog::info("SDL: window-bound '{}' failed ({}), trying next driver", drv, SDL_GetError());
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        continue;  // windowless SDL_CreateRenderer(NULL, ...) needs a window in SDL3
      }
    } else {
      spdlog::info("SDL: hidden window creation failed ({}), trying next driver", SDL_GetError());
      continue;
    }
    if (renderer_ != nullptr) {
      spdlog::info("SDL: rendering with GPU driver '{}'", drv);
      usingGPU_ = true;
      driver = drv.c_str();
      break;
    }
  }

  if (renderer_ == nullptr) {
    if (driver == nullptr && !videoInitialized_) {
      spdlog::info("SDL: no GPU renderer found, rendering on CPU (software renderer, no display/xvfb needed)");
    }
    // Pure CPU path: render straight into our own pixel buffer (or the caller's region), zero copies
    if (externalPixels_ != nullptr) {
      surface_ = SDL_CreateSurfaceFrom(static_cast<int>(pixelWidth_), static_cast<int>(pixelHeight_), SDL_PIXELFORMAT_RGBA32, externalPixels_, externalPitch_);
    } else {
      surface_ = SDL_CreateSurfaceFrom(static_cast<int>(pixelWidth_), static_cast<int>(pixelHeight_), SDL_PIXELFORMAT_RGBA32, pixelBuffer_.data(), static_cast<int>(pixelWidth_ * 4));
    }
    if (surface_ == nullptr) {
      throw std::runtime_error(fmt::format("SDL: failed to create render surface: {0}", SDL_GetError()));
    }
    renderer_ = SDL_CreateSoftwareRenderer(surface_);
    if (renderer_ == nullptr) {
      throw std::runtime_error(fmt::format("SDL: failed to create software renderer: {0}", SDL_GetError()));
    }
    usingGPU_ = false;
  }

  if (usingGPU_) {
    targetTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, static_cast<int>(pixelWidth_), static_cast<int>(pixelHeight_));
    if (targetTexture_ == nullptr) {
      throw std::runtime_error(fmt::format("SDL: failed to create target texture: {0}", SDL_GetError()));
    }
    SDL_SetTextureBlendMode(targetTexture_, SDL_BLENDMODE_NONE);
  }
}

void SDLObserver::resetRenderSurface() {
  spdlog::debug("Initializing SDL render surface. Grid width={0}, height={1}. Pixel width={2}, height={3}", gridWidth_, gridHeight_, pixelWidth_, pixelHeight_);

  bool sizeChanged = renderer_ != nullptr && (pixelWidth_ != lastPixelWidth_ || pixelHeight_ != lastPixelHeight_);
  if ((sharedRenderer_ != nullptr || externalPixels_ != nullptr) && lastPixelWidth_ != 0 &&
      (pixelWidth_ != lastPixelWidth_ || pixelHeight_ != lastPixelHeight_)) {
    throw std::logic_error("Tiled SDL rendering requires the observer size to stay constant");
  }

  if (sharedRenderer_ == nullptr && externalPixels_ == nullptr) {
    pixelBuffer_.assign(static_cast<size_t>(pixelWidth_) * pixelHeight_ * 4, 0);
  } else {
    // tiled modes draw into the caller's composite, no per-observer buffer
    pixelBuffer_.clear();
  }
  // RGBA8 interleaved, tightly packed: channel stride 1, pixel stride 4, row pitch 4*width
  observationStrides_ = {1, 4, pixelWidth_ * 4};
  lastPixelWidth_ = pixelWidth_;
  lastPixelHeight_ = pixelHeight_;

  if (sizeChanged) {
    // Textures and renderers are bound to each other, so when the surface size
    // changes everything renderer-bound must be recreated and sprites reloaded
    for (auto& sprite : spriteTextures_) {
      SDL_DestroyTexture(sprite.second);
    }
    spriteTextures_.clear();
    if (targetTexture_ != nullptr) {
      SDL_DestroyTexture(targetTexture_);
      targetTexture_ = nullptr;
    }
    if (renderer_ != nullptr) {
      SDL_DestroyRenderer(renderer_);
      renderer_ = nullptr;
    }
    if (surface_ != nullptr) {
      SDL_DestroySurface(surface_);
      surface_ = nullptr;
    }
    if (window_ != nullptr) {
      SDL_DestroyWindow(window_);
      window_ = nullptr;
    }
  }
}

SDL_Texture* SDLObserver::getSpriteTexture(const std::string& spriteName) const {
  const auto& textures = sharedSprites_ != nullptr ? *sharedSprites_ : spriteTextures_;
  auto it = textures.find(spriteName);
  if (it == textures.end()) {
    return nullptr;
  }
  return it->second;
}

std::vector<uint8_t> SDLObserver::loadSpriteImage(const std::string& absoluteFilePath, glm::ivec2 tileSize) {
  spdlog::debug("Loading sprite {0}", absoluteFilePath);

  int width, height, channels;
  stbi_uc* pixels = stbi_load(absoluteFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  if (!pixels) {
    throw std::runtime_error(fmt::format("Failed to load texture image {0}.", absoluteFilePath));
  }

  auto outputWidth = static_cast<uint32_t>(tileSize.x);
  auto outputHeight = static_cast<uint32_t>(tileSize.y);

  std::vector<uint8_t> resizedPixels(static_cast<size_t>(outputWidth) * outputHeight * 4);
  auto res = stbir_resize_uint8_generic(pixels, width, height, 0,
                                        resizedPixels.data(), outputWidth, outputHeight, 0, 4,
                                        3, 0,
                                        STBIR_EDGE_CLAMP,
                                        STBIR_FILTER_CATMULLROM,
                                        STBIR_COLORSPACE_LINEAR,
                                        nullptr);
  free(pixels);
  if (!res) {
    throw std::runtime_error(fmt::format("Failed to resize texture image {0}.", absoluteFilePath));
  }
  return resizedPixels;
}

SDL_Texture* SDLObserver::loadSpriteTexture(const std::string& spriteName, const std::string& imageFilename, glm::ivec2 tileSize) {
  auto& textures = sprites();
  auto existing = textures.find(spriteName);
  if (existing != textures.end() && sharedSprites_ != nullptr) {
    return existing->second;  // already loaded by another observer sharing the renderer
  }
  std::string absoluteFilePath = config_.resourceConfig.imagePath + "/" + imageFilename;
  auto outputWidth = static_cast<uint32_t>(tileSize.x);
  auto outputHeight = static_cast<uint32_t>(tileSize.y);
  std::vector<uint8_t> resizedPixels = loadSpriteImage(absoluteFilePath, tileSize);

  SDL_Surface* spriteSurface = SDL_CreateSurfaceFrom(static_cast<int>(outputWidth), static_cast<int>(outputHeight), SDL_PIXELFORMAT_RGBA32, resizedPixels.data(), static_cast<int>(outputWidth * 4));
  if (spriteSurface == nullptr) {
    throw std::runtime_error(fmt::format("SDL: failed to create sprite surface: {0}", SDL_GetError()));
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(activeRenderer(), spriteSurface);
  SDL_DestroySurface(spriteSurface);
  if (texture == nullptr) {
    throw std::runtime_error(fmt::format("SDL: failed to create sprite texture for {0}: {1}", absoluteFilePath, SDL_GetError()));
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

  textures[spriteName] = texture;
  return texture;
}

void SDLObserver::reset(std::shared_ptr<ObjectRules> avatarObject) {
  Observer::reset(avatarObject);
  renderItems_.clear();
  prevCellsValid_ = false;

  if (observerState_ == ObserverState::READY) {
    resetRenderSurface();
  }
}

void SDLObserver::ensureReady() {
  if (observerState_ == ObserverState::RESET) {
    resetRenderSurface();
    if (sharedRenderer_ != nullptr) {
      usingGPU_ = true;
      lazyInit();  // only loads sprites the shared map does not have yet
    } else if (renderer_ == nullptr) {
      initSDL();
      lazyInit();
    }
    observerState_ = ObserverState::READY;
  } else if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer is not in READY state, cannot render");
  }
}

void SDLObserver::prepareFrame() {
  ensureReady();
  updateRenderItems();
  if (hasBaseLayer()) {
    // the base layer already holds the background tile
    renderItems_.erase(std::remove_if(renderItems_.begin(), renderItems_.end(), [](const SDLRenderItem& i) { return i.tile; }), renderItems_.end());
  }
  sortItems();
}

void SDLObserver::discardPreparedFrame() {
  renderItems_.clear();
  grid_->purgeUpdatedLocations(config_.playerId);
}

void SDLObserver::drawBaseLayerOnly() {
  ensureReady();
  updateRenderItems();
  renderItems_.erase(std::remove_if(renderItems_.begin(), renderItems_.end(), [](const SDLRenderItem& i) { return !i.tile; }), renderItems_.end());
  drawFrame();
}

void SDLObserver::setBaseLayer(const uint8_t* pixels, SDL_Texture* texture) {
  basePixels_ = pixels;
  baseTexture_ = texture;
  prevCellsValid_ = false;
}

bool SDLObserver::bucketItemsByCell() {
  const uint32_t tw = static_cast<uint32_t>(config_.tileSize.x), th = static_cast<uint32_t>(config_.tileSize.y);
  if (tw == 0 || th == 0 || pixelWidth_ % tw != 0 || pixelHeight_ % th != 0) return false;
  const uint32_t cellsX = pixelWidth_ / tw, cellsY = pixelHeight_ / th, numCells = cellsX * cellsY;
  cellStart_.assign(numCells + 1, 0);
  itemCell_.resize(renderItems_.size());
  for (size_t i = 0; i < renderItems_.size(); i++) {
    const auto& item = renderItems_[i];
    if (item.texture == nullptr) continue;
    if (item.tile) return false;
    const SDL_FRect& d = item.dst;
    if (d.x < 0 || d.y < 0 || d.x + d.w > static_cast<float>(pixelWidth_) || d.y + d.h > static_cast<float>(pixelHeight_)) return false;
    const uint32_t cx = static_cast<uint32_t>(d.x) / tw, cy = static_cast<uint32_t>(d.y) / th;
    if (d.x + d.w > static_cast<float>((cx + 1) * tw) || d.y + d.h > static_cast<float>((cy + 1) * th)) return false;
    // only quarter turns of a square keep the sprite inside its rect
    if (item.angle != 0.0 && (item.angle != std::round(item.angle / 90.0) * 90.0 || d.w != d.h)) return false;
    itemCell_[i] = cy * cellsX + cx;
    cellStart_[itemCell_[i] + 1]++;
  }
  for (uint32_t c = 0; c < numCells; c++) cellStart_[c + 1] += cellStart_[c];
  cellItems_.resize(cellStart_[numCells]);
  cellDirty_.resize(numCells);
  // stable counting sort: items keep their draw order inside each cell
  auto& fill = itemFill_;
  fill.assign(cellStart_.begin(), cellStart_.end() - 1);
  for (size_t i = 0; i < renderItems_.size(); i++) {
    const auto& item = renderItems_[i];
    if (item.texture == nullptr) continue;
    const uint32_t rgba = static_cast<uint32_t>(item.r) | (static_cast<uint32_t>(item.g) << 8) | (static_cast<uint32_t>(item.b) << 16) | (static_cast<uint32_t>(item.a) << 24);
    cellItems_[fill[itemCell_[i]]++] = CellItem{item.texture, item.src.x, item.src.y, item.src.w, item.src.h,
                                                item.dst.x, item.dst.y, item.dst.w, item.dst.h, static_cast<float>(item.angle), rgba};
  }
  if (prevCellsValid_ && prevCellStart_.size() != cellStart_.size()) prevCellsValid_ = false;
  return true;
}

bool SDLObserver::computeDirtyCells() {
  dirtyCells_.clear();
  fullRedraw_ = true;
  if (std::getenv("GRIDDLY_SDL_NO_DIRTY_CELLS") != nullptr || !bucketItemsByCell()) {
    prevCellsValid_ = false;
    return false;
  }
  const uint32_t numCells = static_cast<uint32_t>(cellStart_.size() - 1);
  fullRedraw_ = !prevCellsValid_;
  for (uint32_t c = 0; c < numCells; c++) {
    bool dirty = fullRedraw_;
    if (!dirty) {
      const uint32_t n = cellStart_[c + 1] - cellStart_[c];
      dirty = n != prevCellStart_[c + 1] - prevCellStart_[c] ||
              !std::equal(cellItems_.begin() + cellStart_[c], cellItems_.begin() + cellStart_[c + 1], prevCellItems_.begin() + prevCellStart_[c]);
    }
    cellDirty_[c] = dirty;
    if (dirty && !fullRedraw_) dirtyCells_.push_back(c);
  }
  // keep only the items of repainted cells
  dirtyItems_.clear();
  for (size_t i = 0; i < renderItems_.size(); i++) {
    if (renderItems_[i].texture != nullptr && cellDirty_[itemCell_[i]]) dirtyItems_.push_back(renderItems_[i]);
  }
  renderItems_.swap(dirtyItems_);
  std::swap(cellStart_, prevCellStart_);
  std::swap(cellItems_, prevCellItems_);
  prevCellsValid_ = true;
  return true;
}

void SDLObserver::restoreWholeBase() {
  auto* dst = static_cast<uint8_t*>(surface_->pixels);
  const size_t rowBytes = static_cast<size_t>(pixelWidth_) * 4;
  for (uint32_t y = 0; y < pixelHeight_; y++) {
    std::memcpy(dst + static_cast<size_t>(y) * surface_->pitch, basePixels_ + y * rowBytes, rowBytes);
  }
}

void SDLObserver::restoreBaseCell(uint32_t cell) {
  const uint32_t tw = static_cast<uint32_t>(config_.tileSize.x), th = static_cast<uint32_t>(config_.tileSize.y);
  const uint32_t cellsX = pixelWidth_ / tw;
  const uint32_t x0 = (cell % cellsX) * tw, y0 = (cell / cellsX) * th;
  auto* dst = static_cast<uint8_t*>(surface_->pixels);
  const size_t baseRow = static_cast<size_t>(pixelWidth_) * 4;
  for (uint32_t y = y0; y < y0 + th; y++) {
    std::memcpy(dst + static_cast<size_t>(y) * surface_->pitch + x0 * 4, basePixels_ + y * baseRow + x0 * 4, static_cast<size_t>(tw) * 4);
  }
}

void SDLObserver::drawPreparedFrame() {
  drawFrame();
  grid_->purgeUpdatedLocations(config_.playerId);
}

uint8_t& SDLObserver::update() {
  prepareFrame();
  drawPreparedFrame();

  if (externalPixels_ != nullptr) return *externalPixels_;
  if (pixelBuffer_.empty()) return emptyPixel_;
  return pixelBuffer_[0];
}

void SDLObserver::sortItems() {
  // Insertion sort: stable, allocation free, and items arrive nearly sorted
  // (background first, then objects in grid order)
  for (size_t i = 1; i < renderItems_.size(); i++) {
    SDLRenderItem item = renderItems_[i];
    size_t j = i;
    while (j > 0 && (renderItems_[j - 1].z > item.z || (renderItems_[j - 1].z == item.z && renderItems_[j - 1].sortY > item.sortY))) {
      renderItems_[j] = renderItems_[j - 1];
      j--;
    }
    renderItems_[j] = item;
  }
}

void SDLObserver::drawItems(SDL_Renderer* r) {
  for (const auto& item : renderItems_) {
    if (item.texture == nullptr) continue;
    SDL_SetTextureColorMod(item.texture, item.r, item.g, item.b);
    SDL_SetTextureAlphaMod(item.texture, item.a);
    if (item.tile) {
      SDL_RenderTextureTiled(r, item.texture, &item.src, 1.0f, &item.dst);
    } else if (item.angle != 0.0) {
      SDL_RenderTextureRotated(r, item.texture, &item.src, &item.dst, item.angle, nullptr, SDL_FLIP_NONE);
    } else {
      SDL_RenderTexture(r, item.texture, &item.src, &item.dst);
    }
  }
}

void SDLObserver::setSharedRenderTarget(SDL_Renderer* renderer, SDL_Texture* target, float offsetX, float offsetY, SpriteTextureMap* sharedSprites) {
  if (renderer_ != nullptr) throw std::logic_error("setSharedRenderTarget must be called before the first update()");
  sharedRenderer_ = renderer;
  sharedTarget_ = target;
  sharedOffX_ = offsetX;
  sharedOffY_ = offsetY;
  sharedSprites_ = sharedSprites;
}

void SDLObserver::setExternalPixelTarget(uint8_t* pixels, int pitch) {
  if (renderer_ != nullptr) throw std::logic_error("setExternalPixelTarget must be called before the first update()");
  externalPixels_ = pixels;
  externalPitch_ = pitch;
}

void SDLObserver::drawFrame() {
  // Tiled GPU mode: draw this env's tile directly into the shared composite
  // target at the tile offset (viewport), so all envs in one frame are a
  // single batch of draw calls on one renderer/target.
  if (sharedRenderer_ != nullptr && sharedTarget_ != nullptr) {
    SDL_SetRenderTarget(sharedRenderer_, sharedTarget_);
    SDL_Rect vp{(int)sharedOffX_, (int)sharedOffY_, (int)pixelWidth_, (int)pixelHeight_};
    SDL_SetRenderViewport(sharedRenderer_, &vp);
    SDL_FRect tile{0, 0, static_cast<float>(pixelWidth_), static_cast<float>(pixelHeight_)};
    if (baseTexture_ != nullptr) {
      SDL_RenderTexture(sharedRenderer_, baseTexture_, nullptr, &tile);
    } else {
      // SDL_RenderClear ignores the viewport in SDL3, so clear the tile with a fill
      SDL_SetRenderDrawBlendMode(sharedRenderer_, SDL_BLENDMODE_NONE);
      SDL_SetRenderDrawColor(sharedRenderer_,
                             static_cast<uint8_t>(clearColor_.r * 255.0f),
                             static_cast<uint8_t>(clearColor_.g * 255.0f),
                             static_cast<uint8_t>(clearColor_.b * 255.0f), 255);
      SDL_RenderFillRect(sharedRenderer_, &tile);
    }
    drawItems(sharedRenderer_);
    SDL_SetRenderViewport(sharedRenderer_, nullptr);  // reset for the compositor pass
    renderItems_.clear();
    prevCellsValid_ = false;  // the whole tile was redrawn outside the incremental path
    return;  // no per-env readback; compositor reads back once
  }

  SDL_SetRenderTarget(renderer_, usingGPU_ ? targetTexture_ : nullptr);

  if (basePixels_ != nullptr && !usingGPU_) {
    // The software renderer draws into surface_ on present, which keeps its
    // pixels between frames: repaint only the cells whose items changed
    if (externalPixels_ != nullptr && computeDirtyCells()) {
      if (fullRedraw_) {
        restoreWholeBase();
      } else {
        for (uint32_t c : dirtyCells_) restoreBaseCell(c);
      }
    } else {
      restoreWholeBase();
    }
  } else {
    SDL_SetRenderDrawColor(renderer_,
                           static_cast<uint8_t>(clearColor_.r * 255.0f),
                           static_cast<uint8_t>(clearColor_.g * 255.0f),
                           static_cast<uint8_t>(clearColor_.b * 255.0f), 255);
    SDL_RenderClear(renderer_);
  }

  drawItems(renderer_);
  renderItems_.clear();

  if (usingGPU_) {
    // Read the frame back into our tightly packed RGBA buffer.
    // Skip the readback when nobody will consume pixelBuffer_ this frame
    // (GRIDDLY_SDL_GPU_NO_READBACK=1): avoids a full GPU->CPU sync per frame.
    if (std::getenv("GRIDDLY_SDL_GPU_NO_READBACK") == nullptr) {
      SDL_Surface* shot = SDL_RenderReadPixels(renderer_, nullptr);
      if (shot != nullptr) {
        SDL_Surface* converted = SDL_ConvertSurface(shot, SDL_PIXELFORMAT_RGBA32);
        if (converted != nullptr) {
          auto* src = static_cast<const uint8_t*>(converted->pixels);
          for (int y = 0; y < converted->h; y++) {
            SDL_memcpy(pixelBuffer_.data() + static_cast<size_t>(y) * pixelWidth_ * 4, src + static_cast<size_t>(y) * converted->pitch, static_cast<size_t>(pixelWidth_) * 4);
          }
          SDL_DestroySurface(converted);
        }
        SDL_DestroySurface(shot);
      }
    }

    // If a visible window was requested, present the frame
    if (window_ != nullptr && !(SDL_GetWindowFlags(window_) & SDL_WINDOW_HIDDEN)) {
      SDL_SetRenderTarget(renderer_, nullptr);
      SDL_RenderTexture(renderer_, targetTexture_, nullptr, nullptr);
      SDL_RenderPresent(renderer_);
    }
  } else {
    // The software renderer only flushes into the surface on present
    SDL_RenderPresent(renderer_);
  }
}

const glm::ivec2 SDLObserver::getTileSize() const {
  return config_.tileSize;
}

void SDLObserver::release() {
  for (auto& sprite : spriteTextures_) {
    SDL_DestroyTexture(sprite.second);
  }
  spriteTextures_.clear();

  if (targetTexture_ != nullptr) {
    SDL_DestroyTexture(targetTexture_);
    targetTexture_ = nullptr;
  }
  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_ != nullptr) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  if (surface_ != nullptr) {
    // surface_ wraps pixelBuffer_, do not free the pixels
    SDL_DestroySurface(surface_);
    surface_ = nullptr;
  }
  if (videoInitialized_) {
    if (--sdlVideoRefCount <= 0) {
      SDL_Quit();  // full quit: SDL_Init was full init
      sdlVideoRefCount = 0;
    }
    videoInitialized_ = false;
  }
  usingGPU_ = false;
}

}  // namespace griddly
