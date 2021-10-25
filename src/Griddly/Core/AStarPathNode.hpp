#include <glm/glm.hpp>

namespace griddly {
class AStarPathNode {
 public:

  AStarPathNode(glm::ivec2 nodeLocation) 
    : score(UINT_MAX), actionId(0), location(nodeLocation), parent(nullptr) {
  }

  AStarPathNode(float nodeScore, uint32_t nodeActionId, glm::ivec2 nodeLocation, std::shared_ptr<AStarPathNode> nodeParent)
      : score(nodeScore), actionId(nodeActionId), location(nodeLocation), parent(nodeParent) {
  }

  bool operator==(const AStarPathNode& other) const {
    return score == other.score;
  }

  bool operator>(const AStarPathNode& other) const {
    return score < other.score;
  }

  bool operator<(const AStarPathNode& other) const {
    return score > other.score;
  }

  const float score;
  const uint32_t actionId;
  const glm::ivec2 location;
  const std::shared_ptr<AStarPathNode> parent;
};
}  // namespace griddly