#pragma once

#include "AStarPathFinder.hpp"

#include <unordered_map>

#include "Grid.hpp"

namespace griddly {

AStarPathFinder::AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> passableObjects, std::unordered_set<std::string> impassableObjects, std::string targetAction, ActionInputsDefinition actionInputs)
    : PathFinder(grid, passableObjects, impassableObjects), targetAction_(targetAction), actionInputs_(actionInputs) {
}


SearchOutput search(glm::ivec2 startLocation, glm::ivec2 endLocation, uint32_t maxDepth) {

  VectorPriorityQueue<AStarPathNode> nodes;

}


}  // namespace griddly