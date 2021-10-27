#pragma once

#include <unordered_map>

#include "PathFinder.hpp"
#include "Grid.hpp"
#include "Util/util.hpp"
#include "AStarPathNode.hpp"
#include "GDY/Actions/Action.hpp"

namespace griddly {


struct SortAStarPathNodes {
  bool operator()(std::shared_ptr<AStarPathNode> a, std::shared_ptr<AStarPathNode> b){
    return a->scoreFromStart > b->scoreFromStart;
  };
};


class AStarPathFinder : public PathFinder {
 public:
  AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> impassableObjects, ActionInputsDefinition actionInputs);

  SearchOutput reconstructPath(std::shared_ptr<AStarPathNode> currentBestNode);

  virtual SearchOutput search(glm::ivec2 startLocation, glm::ivec2 endLocation, glm::ivec2 startOrientationVector, uint32_t maxDepth) override;

  private:
    const std::string targetAction_;
    const ActionInputsDefinition actionInputs_;

};

}  // namespace griddly
