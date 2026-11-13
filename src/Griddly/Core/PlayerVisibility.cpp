#include "PlayerVisibility.hpp"

#include <utility>

#include "GDY/Objects/ObjectRules.hpp"

namespace griddly {

PlayerVisibility::PlayerVisibility(PlayerVisibilityConfig config, uint32_t playerCount, uint32_t width, uint32_t height)
    : config_(std::move(config)), width_(width), height_(height) {
  cells_.assign(playerCount + 1, std::vector<uint8_t>(static_cast<size_t>(width) * height, UNSEEN));
}

int32_t PlayerVisibility::sightOf(const PlayerVisibilityConfig& config, const std::shared_ptr<ObjectRules>& object) {
  auto sightVariable = object->getVariableValue(config.sightVariable);
  return sightVariable != nullptr ? static_cast<int32_t>(*sightVariable) : config.defaultSight;
}

void PlayerVisibility::reveal(std::vector<uint8_t>& cells, uint32_t width, uint32_t height, glm::ivec2 location, int32_t sight) {
  if (sight <= 0) return;
  const int32_t limit = sight * sight + sight;
  for (int32_t dy = -sight; dy <= sight; dy++) {
    const int32_t y = location.y + dy;
    if (y < 0 || y >= static_cast<int32_t>(height)) continue;
    for (int32_t dx = -sight; dx <= sight; dx++) {
      const int32_t x = location.x + dx;
      if (x < 0 || x >= static_cast<int32_t>(width) || dx * dx + dy * dy > limit) continue;
      cells[static_cast<size_t>(y) * width + x] = VISIBLE;
    }
  }
}

void PlayerVisibility::update(const std::vector<std::shared_ptr<ObjectRules>>& objects) {
  if (!config_.enabled) return;
  for (auto& playerCells : cells_) {
    for (auto& cell : playerCells) {
      if (cell == VISIBLE) cell = EXPLORED;
    }
  }
  for (const auto& object : objects) {
    const auto playerId = object->getPlayerId();
    if (playerId == 0 || playerId >= cells_.size()) continue;
    reveal(cells_[playerId], width_, height_, object->getLocation(), sightOf(config_, object));
  }
}

void PlayerVisibility::clear() {
  for (auto& playerCells : cells_) {
    std::fill(playerCells.begin(), playerCells.end(), UNSEEN);
  }
}

const std::vector<uint8_t>& PlayerVisibility::cells(uint32_t playerId) const {
  static const std::vector<uint8_t> none;
  return playerId < cells_.size() ? cells_[playerId] : none;
}

uint8_t PlayerVisibility::at(uint32_t playerId, glm::ivec2 location) const {
  if (playerId >= cells_.size() || location.x < 0 || location.y < 0 || location.x >= static_cast<int32_t>(width_) || location.y >= static_cast<int32_t>(height_)) {
    return UNSEEN;
  }
  return cells_[playerId][static_cast<size_t>(location.y) * width_ + location.x];
}

bool PlayerVisibility::canSee(uint32_t playerId, const std::shared_ptr<ObjectRules>& object) const {
  if (!config_.enabled || playerId == 0) return true;
  const auto owner = object->getPlayerId();
  if (owner == playerId) return true;
  const auto state = at(playerId, object->getLocation());
  if (owner == 0) return state != UNSEEN;
  return state == VISIBLE;
}

}  // namespace griddly
