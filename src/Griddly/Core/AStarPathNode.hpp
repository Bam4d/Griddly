#include <glm/glm.hpp>

namespace griddly {
class AStarPathNode {
 public:

  AStarPathNode(glm::ivec2 nodeLocation) 
    : location(nodeLocation) {
  }

  float scoreFromStart = UINT_MAX;
  float scoreToGoal = UINT_MAX;
  uint32_t actionId = 0;
  std::shared_ptr<AStarPathNode> parent;

  const glm::ivec2 location;
};
}  // namespace griddly