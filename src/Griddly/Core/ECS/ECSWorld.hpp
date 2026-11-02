#pragma once

// ECSWorld: all mutable game state lives in ONE contiguous, fixed-capacity
// arena of trivially-copyable data. Because the arena contains no pointers,
// no strings and no heap handles, the entire game state can be saved/loaded
// with a single memcpy of the arena bytes, and a reset is a single memcpy of
// a previously captured "initial state" snapshot.
//
// The arena is allocated once at construction (capacity derived from the
// level/GDY config) and is never reallocated during play, so there is zero
// object churn while the game runs: entity creation recycles in-arena slots
// via a free list, and components are structure-of-arrays blocks inside the
// arena.
//
// Strings (object names, action names, ...) and behaviour code live in the
// static GDY definitions, NOT in the arena; the arena stores only uint32 ids
// resolved against those definitions. That is what makes the state block
// plain bytes.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "Entity.hpp"

namespace griddly {

struct ECSWorldConfig {
  uint32_t maxEntities = 1024;         // upper bound on live objects
  uint32_t maxVariablesPerEntity = 8;  // int32 variables per object (incl. _x,_y,_dx,_dy,_playerId,_renderTileId)
  uint32_t maxPlayers = 4;
  uint32_t maxGlobalVariables = 64;  // per-player global variables
  uint32_t gridWidth = 0;
  uint32_t gridHeight = 0;
  uint32_t maxDelayedActions = 256;
};

// Plain-old-data record for the delayed action queue (lives in the arena).
struct DelayedActionRecord {
  uint32_t tick = 0;             // tick at which the action fires
  uint32_t playerId = 0;
  EntityId sourceEntity = NULL_ENTITY;
  uint32_t actionId = 0;         // resolved against the GDY action mapping
  int32_t vectorToDestX = 0;
  int32_t vectorToDestY = 0;
  int32_t orientationX = 0;
  int32_t orientationY = 0;
};

class ECSWorld {
 public:
  static constexpr uint32_t MAGIC = 0x47454353u;  // 'GECS'
  static constexpr uint32_t VERSION = 1u;

  // POD header at the start of the arena.
  struct Header {
    uint32_t magic = MAGIC;
    uint32_t version = VERSION;
    ECSWorldConfig config{};
    uint32_t aliveCount = 0;
    uint32_t freeListHead = 0xFFFFFFFFu;
    int32_t tickCount = 0;
    uint32_t delayedActionCount = 0;
    uint32_t globalVariableCount = 0;
    uint64_t stateHash = 0;  // FNV-1a over the payload (excludes this field)
  };

  explicit ECSWorld(ECSWorldConfig config) : config_(config) {
    layout_ = computeLayout(config_);
    arena_.assign(layout_.totalSize, std::byte{0});
    // Placement-new the header so its default member initializers run
    // (magic/version); the rest of the arena stays zeroed.
    Header* h = new (arena_.data()) Header();
    h->config = config_;
    rebuildFreeList();
  }

  // ----- entity lifecycle (no allocation; recycles slots) -----

  EntityId createEntity() {
    if (header()->freeListHead == 0xFFFFFFFFu) {
      return NULL_ENTITY;  // at capacity; caller must size the world for the level
    }
    const uint32_t slot = header()->freeListHead;
    header()->freeListHead = freeNext()[slot];
    freeNext()[slot] = 0xFFFFFFFFu;
    alive()[slot] = 1;
    header()->aliveCount++;
    return EntityId{slot, generations()[slot]};
  }

  void destroyEntity(EntityId entity) {
    if (!isAlive(entity)) return;
    const uint32_t slot = entity.index;
    unlinkFromTile(entity, x(entity), y(entity));  // keep tile lists free of dead slots
    alive()[slot] = 0;
    removed()[slot] = 1;
    generations()[slot]++;  // invalidate outstanding handles
    freeNext()[slot] = header()->freeListHead;
    header()->freeListHead = slot;
    header()->aliveCount--;
  }

  bool isAlive(EntityId entity) const {
    return entity != NULL_ENTITY && entity.index < config_.maxEntities &&
           alive()[entity.index] == 1 && generations()[entity.index] == entity.generation;
  }

  uint32_t aliveCount() const { return header()->aliveCount; }

  // ----- component accessors (direct references into the arena) -----

  int32_t& x(EntityId e) { return transformX()[e.index]; }
  int32_t& y(EntityId e) { return transformY()[e.index]; }
  int32_t& dx(EntityId e) { return orientationX()[e.index]; }
  int32_t& dy(EntityId e) { return orientationY()[e.index]; }
  uint32_t& objectTypeId(EntityId e) { return objectType()[e.index]; }
  uint32_t& playerId(EntityId e) { return playerIdArr()[e.index]; }
  int32_t& zIdx(EntityId e) { return zIndex()[e.index]; }
  uint32_t& renderTileId(EntityId e) { return renderTile()[e.index]; }
  uint8_t& isPlayerAvatar(EntityId e) { return avatarFlag()[e.index]; }
  uint8_t& isRemoved(EntityId e) { return removed()[e.index]; }

  const int32_t& x(EntityId e) const { return transformX()[e.index]; }
  const int32_t& y(EntityId e) const { return transformY()[e.index]; }
  const int32_t& dx(EntityId e) const { return orientationX()[e.index]; }
  const int32_t& dy(EntityId e) const { return orientationY()[e.index]; }
  const uint32_t& objectTypeId(EntityId e) const { return objectType()[e.index]; }
  const uint32_t& playerId(EntityId e) const { return playerIdArr()[e.index]; }
  const int32_t& zIdx(EntityId e) const { return zIndex()[e.index]; }
  const uint32_t& renderTileId(EntityId e) const { return renderTile()[e.index]; }

  int32_t& variable(EntityId e, uint32_t varIdx) { return variables()[e.index * config_.maxVariablesPerEntity + varIdx]; }
  const int32_t& variable(EntityId e, uint32_t varIdx) const { return variables()[e.index * config_.maxVariablesPerEntity + varIdx]; }

  // ----- spatial hash (replaces Grid::occupiedLocations_) -----
  // occupancyHead[tile] = entity index of one occupant (0xFFFFFFFF = empty),
  // linked via nextAtLocation to support multiple objects per tile.

  EntityId topObjectAt(int32_t tx, int32_t ty) const {
    const uint32_t head = occupancyHead()[tileIndex(tx, ty)];
    return head == 0xFFFFFFFFu ? NULL_ENTITY : EntityId{head, generations()[head]};
  }

  // Places a newly created entity on a tile (sets x/y and links it in).
  void placeEntity(EntityId e, int32_t tx, int32_t ty) {
    x(e) = tx;
    y(e) = ty;
    linkToTile(e, tx, ty);
  }

  // Moves an entity between tiles, keeping per-tile lists ordered by z-index.
  void moveEntity(EntityId e, int32_t newX, int32_t newY) {
    const int32_t oldX = x(e), oldY = y(e);
    if (oldX == newX && oldY == newY) return;
    unlinkFromTile(e, oldX, oldY);
    x(e) = newX;
    y(e) = newY;
    linkToTile(e, newX, newY);
  }

  // ----- global variables + tick (also in the arena) -----

  void setGlobalVariableCount(uint32_t count) { header()->globalVariableCount = count; }
  int32_t& globalVariable(uint32_t varIdx, uint32_t player) { return globalVariables()[varIdx * config_.maxPlayers + player]; }
  const int32_t& globalVariable(uint32_t varIdx, uint32_t player) const { return globalVariables()[varIdx * config_.maxPlayers + player]; }

  int32_t tick() const { return header()->tickCount; }
  void setTick(int32_t tick) { header()->tickCount = tick; }

  // ----- delayed action queue (fixed-capacity, in the arena) -----

  bool pushDelayedAction(const DelayedActionRecord& record) {
    if (header()->delayedActionCount >= config_.maxDelayedActions) return false;
    delayedActions()[header()->delayedActionCount++] = record;
    return true;
  }
  uint32_t delayedActionCount() const { return header()->delayedActionCount; }
  const DelayedActionRecord& delayedAction(uint32_t i) const { return delayedActions()[i]; }
  DelayedActionRecord& delayedAction(uint32_t i) { return delayedActions()[i]; }
  void clearDelayedActions() { header()->delayedActionCount = 0; }

  // ----- whole-state byte snapshot / restore / reset -----

  size_t arenaSize() const { return layout_.totalSize; }
  const std::byte* data() const { return arena_.data(); }
  std::byte* data() { return arena_.data(); }

  // Save the ENTIRE game state as a chunk of bytes (header included).
  size_t saveTo(void* dst) const {
    std::memcpy(dst, arena_.data(), arena_.size());
    return arena_.size();
  }

  // Load a state previously written by saveTo. Validates magic/version/
  // capacity so a corrupt or mismatched blob is rejected instead of crashing.
  bool loadFrom(const void* src, size_t size) {
    if (size != arena_.size()) return false;
    const Header* h = reinterpret_cast<const Header*>(src);
    if (h->magic != MAGIC || h->version != VERSION) return false;
    if (std::memcmp(&h->config, &config_, sizeof(ECSWorldConfig)) != 0) return false;
    std::memcpy(arena_.data(), src, size);
    return true;
  }

  // Capture the current arena as the "initial state" (call right after the
  // level is loaded). resetToInitial() then restores it with one memcpy.
  void captureInitialState() {
    initialSnapshot_ = arena_;
  }

  // Empty the world back to its just-constructed state (all slots free,
  // tick 0, queues empty) without reallocating.
  void clear() {
    std::memset(arena_.data() + sizeof(Header), 0, arena_.size() - sizeof(Header));
    Header* h = header();
    h->aliveCount = 0;
    h->tickCount = 0;
    h->delayedActionCount = 0;
    h->globalVariableCount = 0;
    h->stateHash = 0;
    rebuildFreeList();
  }

  // Instant reset: one memcpy of the initial-state bytes.
  void resetToInitial() {
    if (initialSnapshot_.size() == arena_.size()) {
      std::memcpy(arena_.data(), initialSnapshot_.data(), arena_.size());
    }
  }

  bool hasInitialState() const { return initialSnapshot_.size() == arena_.size(); }

  // FNV-1a over the arena payload (header hash field excluded).
  uint64_t computeHash() const {
    uint64_t hash = 1469598103934665603ull;
    const std::byte* p = arena_.data();
    const size_t n = arena_.size();
    const size_t hashFieldOffset = offsetof(Header, stateHash);
    for (size_t i = 0; i < n; ++i) {
      if (i >= hashFieldOffset && i < hashFieldOffset + sizeof(uint64_t)) continue;
      hash ^= static_cast<uint8_t>(p[i]);
      hash *= 1099511628211ull;
    }
    return hash;
  }

  const ECSWorldConfig& config() const { return config_; }

 private:
  struct Layout {
    size_t totalSize = 0;
    size_t generations = 0;
    size_t alive = 0;
    size_t freeNext = 0;
    size_t transformX = 0, transformY = 0, orientationX = 0, orientationY = 0;
    size_t objectType = 0, playerId = 0, zIndex = 0, renderTile = 0, avatarFlag = 0, removed = 0;
    size_t variables = 0;
    size_t occupancyHead = 0;
    size_t nextAtLocation = 0;
    size_t globalVariables = 0;
    size_t delayedActions = 0;
  };

  static size_t alignUp(size_t v, size_t alignment) {
    return (v + alignment - 1) / alignment * alignment;
  }

  static Layout computeLayout(const ECSWorldConfig& c) {
    Layout l;
    size_t off = sizeof(Header);
    auto take = [&](size_t bytes, size_t alignment) {
      off = alignUp(off, alignment);
      const size_t at = off;
      off += bytes;
      return at;
    };
    l.generations = take(sizeof(uint32_t) * c.maxEntities, 4);
    l.alive = take(sizeof(uint8_t) * c.maxEntities, 1);
    l.freeNext = take(sizeof(uint32_t) * c.maxEntities, 4);
    l.transformX = take(sizeof(int32_t) * c.maxEntities, 4);
    l.transformY = take(sizeof(int32_t) * c.maxEntities, 4);
    l.orientationX = take(sizeof(int32_t) * c.maxEntities, 4);
    l.orientationY = take(sizeof(int32_t) * c.maxEntities, 4);
    l.objectType = take(sizeof(uint32_t) * c.maxEntities, 4);
    l.playerId = take(sizeof(uint32_t) * c.maxEntities, 4);
    l.zIndex = take(sizeof(int32_t) * c.maxEntities, 4);
    l.renderTile = take(sizeof(uint32_t) * c.maxEntities, 4);
    l.avatarFlag = take(sizeof(uint8_t) * c.maxEntities, 1);
    l.removed = take(sizeof(uint8_t) * c.maxEntities, 1);
    l.variables = take(sizeof(int32_t) * c.maxEntities * c.maxVariablesPerEntity, 4);
    l.occupancyHead = take(sizeof(uint32_t) * c.gridWidth * c.gridHeight, 4);
    l.nextAtLocation = take(sizeof(uint32_t) * c.maxEntities, 4);
    l.globalVariables = take(sizeof(int32_t) * c.maxGlobalVariables * c.maxPlayers, 4);
    l.delayedActions = take(sizeof(DelayedActionRecord) * c.maxDelayedActions, 8);
    l.totalSize = alignUp(off, 8);
    return l;
  }

  template <typename T>
  T* at(size_t offset) {
    return reinterpret_cast<T*>(arena_.data() + offset);
  }
  template <typename T>
  const T* at(size_t offset) const {
    return reinterpret_cast<const T*>(arena_.data() + offset);
  }

  Header* header() { return at<Header>(0); }
  const Header* header() const { return at<Header>(0); }

  uint32_t* generations() { return at<uint32_t>(layout_.generations); }
  const uint32_t* generations() const { return at<uint32_t>(layout_.generations); }
  uint8_t* alive() { return at<uint8_t>(layout_.alive); }
  const uint8_t* alive() const { return at<uint8_t>(layout_.alive); }
  uint32_t* freeNext() { return at<uint32_t>(layout_.freeNext); }
  int32_t* transformX() { return at<int32_t>(layout_.transformX); }
  const int32_t* transformX() const { return at<int32_t>(layout_.transformX); }
  int32_t* transformY() { return at<int32_t>(layout_.transformY); }
  const int32_t* transformY() const { return at<int32_t>(layout_.transformY); }
  int32_t* orientationX() { return at<int32_t>(layout_.orientationX); }
  const int32_t* orientationX() const { return at<int32_t>(layout_.orientationX); }
  int32_t* orientationY() { return at<int32_t>(layout_.orientationY); }
  const int32_t* orientationY() const { return at<int32_t>(layout_.orientationY); }
  uint32_t* objectType() { return at<uint32_t>(layout_.objectType); }
  const uint32_t* objectType() const { return at<uint32_t>(layout_.objectType); }
  uint32_t* playerIdArr() { return at<uint32_t>(layout_.playerId); }
  const uint32_t* playerIdArr() const { return at<uint32_t>(layout_.playerId); }
  int32_t* zIndex() { return at<int32_t>(layout_.zIndex); }
  const int32_t* zIndex() const { return at<int32_t>(layout_.zIndex); }
  uint32_t* renderTile() { return at<uint32_t>(layout_.renderTile); }
  const uint32_t* renderTile() const { return at<uint32_t>(layout_.renderTile); }
  uint8_t* avatarFlag() { return at<uint8_t>(layout_.avatarFlag); }
  uint8_t* removed() { return at<uint8_t>(layout_.removed); }
  const uint8_t* removed() const { return at<uint8_t>(layout_.removed); }
  int32_t* variables() { return at<int32_t>(layout_.variables); }
  const int32_t* variables() const { return at<int32_t>(layout_.variables); }
  uint32_t* occupancyHead() { return at<uint32_t>(layout_.occupancyHead); }
  const uint32_t* occupancyHead() const { return at<uint32_t>(layout_.occupancyHead); }
  uint32_t* nextAtLocation() { return at<uint32_t>(layout_.nextAtLocation); }
  int32_t* globalVariables() { return at<int32_t>(layout_.globalVariables); }
  const int32_t* globalVariables() const { return at<int32_t>(layout_.globalVariables); }
  DelayedActionRecord* delayedActions() { return at<DelayedActionRecord>(layout_.delayedActions); }
  const DelayedActionRecord* delayedActions() const { return at<DelayedActionRecord>(layout_.delayedActions); }

  uint32_t tileIndex(int32_t tx, int32_t ty) const {
    return static_cast<uint32_t>(ty) * config_.gridWidth + static_cast<uint32_t>(tx);
  }

  void rebuildFreeList() {
    // All slots start free, linked in index order.
    for (uint32_t i = 0; i < config_.maxEntities; ++i) {
      generations()[i] = 1;  // generation 0 is reserved for NULL_ENTITY semantics
      alive()[i] = 0;
      freeNext()[i] = i + 1;
    }
    freeNext()[config_.maxEntities - 1] = 0xFFFFFFFFu;
    header()->freeListHead = 0;
    header()->aliveCount = 0;
  }

  void linkToTile(EntityId e, int32_t tx, int32_t ty) {
    const uint32_t tile = tileIndex(tx, ty);
    const uint32_t head = occupancyHead()[tile];
    if (head == e.index) return;  // already linked here; never self-link
    // Insert ordered by z-index (descending) so topObjectAt returns highest z.
    if (head == 0xFFFFFFFFu || zIndex()[head] <= zIdx(e)) {
      nextAtLocation()[e.index] = head;
      occupancyHead()[tile] = e.index;
    } else {
      uint32_t cur = head;
      while (nextAtLocation()[cur] != 0xFFFFFFFFu && zIndex()[nextAtLocation()[cur]] > zIdx(e)) {
        cur = nextAtLocation()[cur];
      }
      nextAtLocation()[e.index] = nextAtLocation()[cur];
      nextAtLocation()[cur] = e.index;
    }
  }

  void unlinkFromTile(EntityId e, int32_t tx, int32_t ty) {
    const uint32_t tile = tileIndex(tx, ty);
    uint32_t* link = &occupancyHead()[tile];
    while (*link != 0xFFFFFFFFu) {
      if (*link == e.index) {
        *link = nextAtLocation()[e.index];
        nextAtLocation()[e.index] = 0xFFFFFFFFu;
        return;
      }
      link = &nextAtLocation()[*link];
    }
  }

  ECSWorldConfig config_;
  Layout layout_;
  std::vector<std::byte> arena_;           // the ONE block of ECS memory
  std::vector<std::byte> initialSnapshot_; // captured at level load for instant reset
};

}  // namespace griddly
