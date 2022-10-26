#pragma once

#include "AStarPathNode.hpp"
#include "GDY/Actions/Action.hpp"
#include "Grid.hpp"
#include "PathFinder.hpp"
#include "Util/util.hpp"

namespace griddly {

struct SortAStarPathNodes {
  bool operator()(const std::shared_ptr<AStarPathNode>& a, const std::shared_ptr<AStarPathNode>& b) {
    return a->scoreFromStart > b->scoreFromStart;
  };
};


class AStarPathFinder : public PathFinder {
 public:
  AStarPathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects, ActionInputsDefinition actionInputs, PathFinderMode mode);

  SearchOutput reconstructPath(const std::shared_ptr<AStarPathNode>& currentBestNode);

  SearchOutput search(glm::ivec2 startLocation, glm::ivec2 endLocation, glm::ivec2 startOrientationVector, uint32_t maxDepth) override;

 private:
  const std::string targetAction_;
  const ActionInputsDefinition actionInputs_;
};

}  // namespace griddly
