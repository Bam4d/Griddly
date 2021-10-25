#pragma once

#include <unordered_map>

#include "PathFinder.hpp"
#include "Grid.hpp"
#include "Util/util.hpp"
#include "AStarPathNode.hpp"

namespace griddly {

class AStarPathFinder : public PathFinder {
 public:
  AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> impassableObjects, ActionInputsDefinition actionInputs);

  SearchOutput reconstructPath(std::shared_ptr<AStarPathNode> currentBestNode);

  virtual SearchOutput search(glm::ivec2 startLocation, glm::ivec2 endLocation, uint32_t maxDepth) override;

  private:
    const std::string targetAction_;
    const ActionInputsDefinition actionInputs_;


};

}  // namespace griddly
