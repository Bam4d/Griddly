#include "SDLObserver.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <spdlog/spdlog.h>

#include <algorithm>
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
  if (std::getenv("GRIDDLY_SDL_FORCE_CPU") == nullptr) {
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
    // Pure CPU path: render straight into our own pixel buffer, zero copies
    surface_ = SDL_CreateSurfaceFrom(static_cast<int>(pixelWidth_), static_cast<int>(pixelHeight_), SDL_PIXELFORMAT_RGBA32, pixelBuffer_.data(), static_cast<int>(pixelWidth_ * 4));
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

  pixelBuffer_.assign(static_cast<size_t>(pixelWidth_) * pixelHeight_ * 4, 0);
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
  auto it = spriteTextures_.find(spriteName);
  if (it == spriteTextures_.end()) {
    return nullptr;
  }
  return it->second;
}

SDL_Texture* SDLObserver::loadSpriteTexture(const std::string& spriteName, const std::string& imageFilename, glm::ivec2 tileSize) {
  std::string absoluteFilePath = config_.resourceConfig.imagePath + "/" + imageFilename;
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

  SDL_Surface* spriteSurface = SDL_CreateSurfaceFrom(static_cast<int>(outputWidth), static_cast<int>(outputHeight), SDL_PIXELFORMAT_RGBA32, resizedPixels.data(), static_cast<int>(outputWidth * 4));
  if (spriteSurface == nullptr) {
    throw std::runtime_error(fmt::format("SDL: failed to create sprite surface: {0}", SDL_GetError()));
  }
  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, spriteSurface);
  SDL_DestroySurface(spriteSurface);
  if (texture == nullptr) {
    throw std::runtime_error(fmt::format("SDL: failed to create sprite texture for {0}: {1}", absoluteFilePath, SDL_GetError()));
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);

  spriteTextures_[spriteName] = texture;
  return texture;
}

void SDLObserver::reset(std::shared_ptr<ObjectRules> avatarObject) {
  Observer::reset(avatarObject);
  renderItems_.clear();

  if (observerState_ == ObserverState::READY) {
    resetRenderSurface();
  }
}

uint8_t& SDLObserver::update() {
  if (observerState_ == ObserverState::RESET) {
    resetRenderSurface();
    if (renderer_ == nullptr) {
      initSDL();
      lazyInit();
    }
    observerState_ = ObserverState::READY;
  } else if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer is not in READY state, cannot render");
  }

  updateRenderItems();
  drawFrame();

  grid_->purgeUpdatedLocations(config_.playerId);

  return pixelBuffer_[0];
}

void SDLObserver::drawFrame() {
  SDL_SetRenderTarget(renderer_, usingGPU_ ? targetTexture_ : nullptr);

  SDL_SetRenderDrawColor(renderer_,
                         static_cast<uint8_t>(clearColor_.r * 255.0f),
                         static_cast<uint8_t>(clearColor_.g * 255.0f),
                         static_cast<uint8_t>(clearColor_.b * 255.0f), 255);
  SDL_RenderClear(renderer_);

  std::stable_sort(renderItems_.begin(), renderItems_.end(), [](const SDLRenderItem& a, const SDLRenderItem& b) {
    if (a.z != b.z) return a.z < b.z;
    return a.sortY < b.sortY;
  });

  for (const auto& item : renderItems_) {
    if (item.texture == nullptr) continue;
    SDL_SetTextureColorMod(item.texture, item.r, item.g, item.b);
    SDL_SetTextureAlphaMod(item.texture, item.a);
    if (item.tile) {
      SDL_RenderTextureTiled(renderer_, item.texture, &item.src, 1.0f, &item.dst);
    } else if (item.angle != 0.0) {
      SDL_RenderTextureRotated(renderer_, item.texture, &item.src, &item.dst, item.angle, nullptr, SDL_FLIP_NONE);
    } else {
      SDL_RenderTexture(renderer_, item.texture, &item.src, &item.dst);
    }
  }
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
