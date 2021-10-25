#include "AStarPathFinder.hpp"
#include <limits>
#include <memory>

#include <unordered_map>

#include "Grid.hpp"

namespace griddly {

AStarPathFinder::AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> impassableObjects, ActionInputsDefinition actionInputs)
    : PathFinder(grid, impassableObjects), actionInputs_(actionInputs) {
}

SearchOutput AStarPathFinder::reconstructPath(std::shared_ptr<AStarPathNode> currentBestNode) {
  if(currentBestNode->parent == nullptr) {
    return {currentBestNode->actionId};
  }
  return {0};
}

SearchOutput AStarPathFinder::search(glm::ivec2 startLocation, glm::ivec2 endLocation, uint32_t maxDepth) {

  VectorPriorityQueue<std::shared_ptr<AStarPathNode>> searchedNodes;

  auto startNode = std::make_shared<AStarPathNode>(AStarPathNode(startLocation));

  searchedNodes.push(startNode);

  std::unordered_map<glm::ivec2, float> scoreToGoal;

  while(!searchedNodes.empty()) {
  
    auto currentBestNode = searchedNodes.top();

    searchedNodes.pop();

    if(currentBestNode->location == endLocation) {
      return reconstructPath(currentBestNode);
    }

    for(auto& inputMapping : actionInputs_.inputMappings) {
      auto actionId = inputMapping.first;
      auto mapping = inputMapping.second;

      // TODO: calculate this with respect to action/object orientation
      auto vectorToDest = mapping.vectorToDest;
      auto nextLocation = currentBestNode->location + vectorToDest;

      // If this location is passable
      auto objectsAtNextLocation = grid_->getObjectsAt(nextLocation);
      bool passable = true;
      for (auto object : objectsAtNextLocation) {
        if(impassableObjects_.find(object.second->getObjectName()) != impassableObjects_.end()) {
          passable = false;
          break;
        }
      }

      if(passable) {
        float neighbourScoreToGoal = UINT_MAX;

        if(scoreToGoal.find(nextLocation) != scoreToGoal.end()) {
          neighbourScoreToGoal = scoreToGoal.at(nextLocation);
        } 

        auto nextScoreToGoal = scoreToGoal[currentBestNode->location] + glm::length((glm::vec2)mapping.vectorToDest);

        if(nextScoreToGoal < neighbourScoreToGoal) {
          // We have found a better path
          scoreToGoal[nextLocation] = nextScoreToGoal;
          auto nodeScore = nextScoreToGoal + glm::distance((glm::vec2)endLocation, (glm::vec2)nextLocation);
          searchedNodes.push(std::make_shared<AStarPathNode>(AStarPathNode(nodeScore, actionId, nextLocation, currentBestNode)));
        }
      }

    }
  }

  return SearchOutput();

}


}  // namespace griddly