#pragma once

#include <unordered_map>

#include "PathFinder.hpp"
#include "Grid.hpp"
#include "Util/util.hpp"
#include "AStarPathNode.hpp"

namespace griddly {

struct PathNode {

}

class AStarPathFinder : public PathFinder {
 public:
  AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> passableObjects, std::unordered_set<std::string> impassableObjects, std::string targetAction, ActionInputsDefinition actionInputs);

  virtual SearchOutput search(glm::ivec2 startLocation, glm::ivec2 endLocation, uint32_t maxDepth) override;

  private:
    const std::string targetAction_;
    const ActionInputsDefinition actionInputs_;


};

}  // namespace griddly
