#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace griddly {

class ObjectRules;

// Fog of war shared by everything a player can see or query: each player only sees the cells
// within the sight radius of their own objects. Cells seen before stay "explored". Enabled by
// `Environment.Player.FogOfWar` in the GDY; observers with FogOfWar and the per-player game state
// both read it, so renders and state agree.
struct PlayerVisibilityConfig {
  bool enabled = false;
  // object variable holding an object's sight radius (cells); objects without it use defaultSight
  std::string sightVariable = "sight";
  int32_t defaultSight = 0;
};

enum CellVisibility : uint8_t {
  UNSEEN = 0,
  EXPLORED = 1,
  VISIBLE = 2,
};

class PlayerVisibility {
 public:
  PlayerVisibility() = default;
  PlayerVisibility(PlayerVisibilityConfig config, uint32_t playerCount, uint32_t width, uint32_t height);

  bool enabled() const { return config_.enabled; }
  const PlayerVisibilityConfig& config() const { return config_; }

  // Recompute what each player sees now (cells visible last update become explored)
  void update(const std::vector<std::shared_ptr<ObjectRules>>& objects);

  // Mark every cell unseen (new episode)
  void clear();

  // Cell states for a player (1-based), row major width x height; empty for unknown players
  const std::vector<uint8_t>& cells(uint32_t playerId) const;

  uint8_t at(uint32_t playerId, glm::ivec2 location) const;

  // Would `playerId` see this object right now? Own objects always; terrain / neutral objects once
  // their cell has been explored; other players' objects only while their cell is visible.
  bool canSee(uint32_t playerId, const std::shared_ptr<ObjectRules>& object) const;

  // Cells within `sight` of `location` (a disc; r^2 + r keeps the edge cells of each row round)
  static void reveal(std::vector<uint8_t>& cells, uint32_t width, uint32_t height, glm::ivec2 location, int32_t sight);

  // Sight radius of an object for this config
  static int32_t sightOf(const PlayerVisibilityConfig& config, const std::shared_ptr<ObjectRules>& object);

 private:
  PlayerVisibilityConfig config_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  std::vector<std::vector<uint8_t>> cells_;  // index 0 unused (player ids start at 1)
};

}  // namespace griddly
