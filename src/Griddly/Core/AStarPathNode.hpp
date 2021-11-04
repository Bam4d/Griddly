#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <limits>

namespace griddly {
class AStarPathNode {
 public:

  AStarPathNode(glm::ivec2 nodeLocation, glm::ivec2 nodeOrientationVector) 
    : location(nodeLocation), orientationVector(nodeOrientationVector) {
  }

  float scoreFromStart = std::numeric_limits<float>::max();
  float scoreToGoal = std::numeric_limits<float>::max();
  uint32_t actionId = 0;
  std::shared_ptr<AStarPathNode> parent;

  const glm::ivec2 location;
  const glm::ivec2 orientationVector;
};
}  // namespace griddly