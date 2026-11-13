#pragma once
#include <SDL3/SDL.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
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

enum class HighlightStyle {
  OUTLINE,
  GLOW,
  SHADOW,
};

struct SDLObserverConfig : ObserverConfig {
  ResourceConfig resourceConfig{};
  ShaderVariableConfig shaderVariableConfig{};

  bool highlightPlayers = false;
  // OUTLINE: a solid player-coloured copy of the sprite drawn behind it.
  // GLOW: a soft player-coloured radial glow centred on the object.
  // SHADOW: the glow squashed into an ellipse under the bottom of the sprite.
  HighlightStyle highlightStyle = HighlightStyle::OUTLINE;
  // size of the highlight relative to the sprite
  float highlightScale = 1.1f;
  std::vector<glm::vec3> playerColors{};
  glm::ivec2 tileSize = {24, 24};
  glm::vec3 globalObserverAvatarHighlightColor{0.3, 0.3, 0.3};

  GlobalObserverAvatarMode globalObserverAvatarMode = GlobalObserverAvatarMode::NONE;
};

using SpriteTextureMap = std::map<std::string, SDL_Texture*>;

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
  // update() split in two for the tiled GPU renderer: prepareFrame() builds
  // the render items (CPU only, safe to run on worker threads once the
  // observer is ready), drawPreparedFrame() issues the draw calls and must run
  // on the renderer's thread. ensureReady() does first-frame SDL setup.
  void ensureReady();
  void prepareFrame();
  void drawPreparedFrame();
  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;
  void reset(std::shared_ptr<ObjectRules> avatarObject = nullptr) override;
  void release() override;

  const glm::ivec2 getTileSize() const;

  // Sprite texture registry (replaces the Vulkan sprite array layers)
  SDL_Texture* getSpriteTexture(const std::string& spriteName) const;
  SDL_Texture* loadSpriteTexture(const std::string& spriteName, const std::string& imageFilename, glm::ivec2 tileSize);
  // Load an image file as tightly packed RGBA8 resized to tileSize (shared with the CUDA tiled renderer)
  static std::vector<uint8_t> loadSpriteImage(const std::string& absoluteFilePath, glm::ivec2 tileSize);
  // White radial glow (opaque centre, fast fall-off), tinted per player with the item colour
  SDL_Texture* loadGlowTexture(const std::string& spriteName, glm::ivec2 tileSize);
  static std::vector<uint8_t> makeGlowImage(glm::ivec2 tileSize);

  bool isUsingGPU() const { return usingGPU_; }
  SDL_Texture* getTargetTexture() const { return usingGPU_ ? targetTexture_ : nullptr; }
  SDL_Renderer* getRenderer() const { return renderer_; }

  // Tiled rendering (TiledSDLRenderer). Both must be called before the first update().
  //
  // GPU: draw with a renderer owned by the caller into `target` at the given
  // offset, with sprite textures shared between all observers using that
  // renderer. SDL textures belong to one renderer, so per-observer renderers
  // cannot be composited on the GPU.
  void setSharedRenderTarget(SDL_Renderer* renderer, SDL_Texture* target, float offsetX, float offsetY, SpriteTextureMap* sharedSprites);
  // CPU: the software renderer draws straight into a caller-owned RGBA8 region
  // (top-left at `pixels`, row pitch `pitch` bytes), e.g. this env's rect of a
  // tiled composite, so no copy is needed afterwards.
  void setExternalPixelTarget(uint8_t* pixels, int pitch);

  // The clear colour + background tile are identical every frame. When a base
  // layer is set the observer skips them: CPU mode copies `pixels` (this env's
  // pre-rendered base, tightly packed RGBA8) into its target before drawing,
  // GPU mode draws `texture` (blend NONE) instead of clear + tiled background.
  void setBaseLayer(const uint8_t* pixels, SDL_Texture* texture);
  // Draw only the clear colour + background tile into this observer's target
  // (used once to capture the base layer)
  void drawBaseLayerOnly();
  bool hasBaseLayer() const { return basePixels_ != nullptr || baseTexture_ != nullptr; }

  // Items built by prepareFrame(), sorted in draw order (for batching callers)
  const std::vector<SDLRenderItem>& preparedItems() const { return renderItems_; }
  // Drop the prepared items without drawing them (caller drew them itself)
  void discardPreparedFrame();

  // Incremental repaint: compares the prepared items with the previous
  // frame's, cell by cell (cells are tileSize blocks), and keeps only the
  // items of cells that changed. Returns false (and forgets the previous
  // frame) when an item is not inside a single cell; the caller must then
  // redraw everything. After true, fullRedraw() says whether every cell must
  // be repainted (first frame or after a reset), else dirtyCells() lists them.
  // The caller is responsible for restoring the base layer under those cells.
  bool computeDirtyCells();
  // Incremental item building (tiled renderers): when enabled and the
  // observer supports it, prepareFrame() rebuilds only the cells around grid
  // locations that changed since the last frame, using a private change set
  // fed by Grid::invalidateLocation. Falls back to a full rebuild after a
  // grid reset/restore or when an item is not cell-aligned.
  void enableIncrementalItems(bool enable);
  // Whether the last prepareFrame() built items incrementally
  bool lastFrameIncremental() const { return incrementalFrame_; }
  bool fullRedraw() const { return fullRedraw_; }
  const std::vector<uint32_t>& dirtyCells() const { return dirtyCells_; }
  glm::vec3 clearColor() const { return clearColor_; }

  // Actual size of the RGBA pixel buffer (for safe zero-copy views from JS)
  size_t bufferSize() const { return pixelBuffer_.size(); }

  // Read-only view of the RGBA8 pixel buffer (used by the tiled compositor)
  const std::vector<uint8_t>& pixelBuffer() const { return pixelBuffer_; }

 protected:
  // Subclasses load their sprite definitions here, renderer_ is guaranteed to exist
  virtual void lazyInit() = 0;
  // Subclasses fill renderItems_ for the current frame
  virtual void updateRenderItems() = 0;
  // Incremental item building hooks (SDLGridObserver implements them)
  virtual bool supportsIncrementalItems() const { return false; }
  // Append the items of every object at `location`, in draw order
  virtual void appendItemsAt(glm::ivec2 location, std::vector<SDLRenderItem>& out) { (void)location; (void)out; }

  void resetRenderSurface();
  void drawFrame();
  void drawItems(SDL_Renderer* r);
  void sortItems();

  SDL_Renderer* renderer_ = nullptr;
  SDL_Window* window_ = nullptr;
  SDL_Texture* targetTexture_ = nullptr;  // GPU path: render into this, then read back
  SDL_Surface* surface_ = nullptr;         // CPU path: software renderer draws straight into pixelBuffer_

  // Tiled GPU mode: drawFrame renders into the shared composite target at
  // (sharedOffX_, sharedOffY_) instead of targetTexture_
  SDL_Renderer* sharedRenderer_ = nullptr;
  SDL_Texture* sharedTarget_ = nullptr;
  float sharedOffX_ = 0, sharedOffY_ = 0;

  SpriteTextureMap* sharedSprites_ = nullptr;

  uint8_t* externalPixels_ = nullptr;
  int externalPitch_ = 0;
  uint8_t emptyPixel_ = 0;

  const uint8_t* basePixels_ = nullptr;
  SDL_Texture* baseTexture_ = nullptr;

  // Tiled modes keep their pixels between frames, so only pixel cells
  // (tileSize blocks) whose items changed are repainted. Valid while every
  // item lies inside one cell; otherwise the frame is redrawn.
  struct CellItem {
    SDL_Texture* texture;
    float sx, sy, sw, sh;
    float dx, dy, dw, dh;
    float angle;
    uint32_t rgba;
    bool operator==(const CellItem& o) const {
      return texture == o.texture && sx == o.sx && sy == o.sy && sw == o.sw && sh == o.sh && dx == o.dx && dy == o.dy &&
             dw == o.dw && dh == o.dh && angle == o.angle && rgba == o.rgba;
    }
  };
  static CellItem toCellItem(const SDLRenderItem& item);
  // cell of an item, or -1 when it is not inside a single cell
  int32_t cellOf(const SDLRenderItem& item) const;
  uint32_t cellsX() const { return pixelWidth_ / static_cast<uint32_t>(config_.tileSize.x); }
  uint32_t numCells() const;
  // full rebuild of the per-cell cache from renderItems_; false if an item is not cell-aligned
  bool rebuildCellCache();
  // incremental: rebuild the cells around changed locations; false = do a full rebuild instead
  bool prepareIncremental();
  void restoreBaseCell(uint32_t cell);
  void restoreWholeBase();
  std::vector<uint32_t> dirtyCells_;
  bool fullRedraw_ = true;
  std::vector<std::vector<CellItem>> cellCache_;        // items drawn last frame, per cell, in draw order
  std::vector<std::vector<CellItem>> stagedCells_;      // scratch for the cells rebuilt this frame
  std::vector<uint32_t> stagedCellIds_;
  std::vector<uint8_t> cellMark_;
  std::vector<SDLRenderItem> cellScratch_;
  bool prevCellsValid_ = false;
  bool incrementalItems_ = false;
  bool incrementalFrame_ = false;
  bool changeListenerRegistered_ = false;
  uint64_t seenStateGeneration_ = 0;
  std::unordered_set<glm::ivec2> changedLocations_;

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

  SDL_Texture* createSpriteTexture(const std::string& spriteName, const std::vector<uint8_t>& pixels, glm::ivec2 tileSize, const std::string& source);

  SpriteTextureMap& sprites() { return sharedSprites_ != nullptr ? *sharedSprites_ : spriteTextures_; }
  SDL_Renderer* activeRenderer() const { return sharedRenderer_ != nullptr ? sharedRenderer_ : renderer_; }

  SpriteTextureMap spriteTextures_;
};

// When true, SDL observers create a visible window (used by the WASM/JS app,
// where SDL renders into the page's <canvas> element via WebGL/WebGPU)
void setSDLWindowVisible(bool visible);

}  // namespace griddly
