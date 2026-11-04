#pragma once
#include <SDL3/SDL.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../../Grid.hpp"
#include "../Observer.hpp"
#include "../TensorObservationInterface.hpp"

namespace griddly {

struct ResourceConfig {
  std::string gdyPath = "";
  std::string imagePath = "";
  // Kept for config compatibility, no longer used (no shaders with SDL)
  std::string shaderPath = "";
};

struct ShaderVariableConfig {
  std::vector<std::string> exposedGlobalVariables = {"_steps"};
  std::vector<std::string> exposedObjectVariables = {};
};

enum class GlobalObserverAvatarMode {
  NONE,
  GRAYSCALE_INVISIBLE,
  DARKEN_INVISIBLE,
  REMOVE_INVISIBLE,
  HIGHLIGHT_VISIBLE,
};

struct SDLObserverConfig : ObserverConfig {
  ResourceConfig resourceConfig{};
  ShaderVariableConfig shaderVariableConfig{};

  bool highlightPlayers = false;
  std::vector<glm::vec3> playerColors{};
  glm::ivec2 tileSize = {24, 24};
  glm::vec3 globalObserverAvatarHighlightColor{0.3, 0.3, 0.3};

  GlobalObserverAvatarMode globalObserverAvatarMode = GlobalObserverAvatarMode::NONE;
};

// A single textured quad (or tiled fill) to draw for one frame
struct SDLRenderItem {
  SDL_Texture* texture = nullptr;
  SDL_FRect src{0, 0, 0, 0};
  SDL_FRect dst{0, 0, 0, 0};
  uint8_t r = 255, g = 255, b = 255, a = 255;
  double angle = 0.0;  // degrees, clockwise (SDL convention)
  bool tile = false;   // tile the src rect across dst
  float z = 0.0f;
  float sortY = 0.0f;  // secondary sort key (isometric depth)
};

class SDLObserver : public Observer, public TensorObservationInterface {
 public:
  explicit SDLObserver(std::shared_ptr<Grid> grid, SDLObserverConfig& config);
  ~SDLObserver() override;

  uint8_t& update() override;
  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;
  void reset(std::shared_ptr<ObjectRules> avatarObject = nullptr) override;
  void release() override;

  const glm::ivec2 getTileSize() const;

  // Sprite texture registry (replaces the Vulkan sprite array layers)
  SDL_Texture* getSpriteTexture(const std::string& spriteName) const;
  SDL_Texture* loadSpriteTexture(const std::string& spriteName, const std::string& imageFilename, glm::ivec2 tileSize);

  bool isUsingGPU() const { return usingGPU_; }

 protected:
  // Subclasses load their sprite definitions here, renderer_ is guaranteed to exist
  virtual void lazyInit() = 0;
  // Subclasses fill renderItems_ for the current frame
  virtual void updateRenderItems() = 0;

  void resetRenderSurface();
  void drawFrame();

  SDL_Renderer* renderer_ = nullptr;
  SDL_Window* window_ = nullptr;
  SDL_Texture* targetTexture_ = nullptr;  // GPU path: render into this, then read back
  SDL_Surface* surface_ = nullptr;         // CPU path: software renderer draws straight into pixelBuffer_

  std::vector<uint8_t> pixelBuffer_;  // RGBA8, tightly packed, this is what update() returns

  uint32_t pixelWidth_ = 0;
  uint32_t pixelHeight_ = 0;
  uint32_t lastPixelWidth_ = 0;
  uint32_t lastPixelHeight_ = 0;

  bool usingGPU_ = false;
  bool videoInitialized_ = false;

  glm::vec3 clearColor_{0.0f, 0.0f, 0.0f};
  std::vector<SDLRenderItem> renderItems_;
  std::vector<glm::vec4> playerColors_;

  SDLObserverConfig config_;

 private:
  void initSDL();
  void destroySDL();

  std::map<std::string, SDL_Texture*> spriteTextures_;
};

// When true, SDL observers create a visible window (used by the WASM/JS app,
// where SDL renders into the page's <canvas> element via WebGL/WebGPU)
void setSDLWindowVisible(bool visible);

}  // namespace griddly
