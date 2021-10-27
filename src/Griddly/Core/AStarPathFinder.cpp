#include "AStarPathFinder.hpp"
#include <limits>
#include <memory>
#include <spdlog/spdlog.h>
#include <unordered_map>

#include "Grid.hpp"

namespace griddly {

AStarPathFinder::AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> impassableObjects, ActionInputsDefinition actionInputs)
    : PathFinder(grid, impassableObjects), actionInputs_(actionInputs) {
}

SearchOutput AStarPathFinder::reconstructPath(std::shared_ptr<AStarPathNode> currentBestNode) {
  if(currentBestNode->parent == nullptr) {
    return {currentBestNode->actionId};
  } else {
    return reconstructPath(currentBestNode->parent);
  }
  return {0};
}

SearchOutput AStarPathFinder::search(glm::ivec2 startLocation, glm::ivec2 endLocation, uint32_t maxDepth) {

  VectorPriorityQueue<std::shared_ptr<AStarPathNode>> orderedBestNodes;
  std::unordered_map<glm::ivec2, std::shared_ptr<AStarPathNode>> nodes;

  auto startNode = std::make_shared<AStarPathNode>(AStarPathNode(startLocation));
  startNode->scoreFromStart = glm::distance((glm::vec2)endLocation, (glm::vec2)startLocation);
  startNode->scoreToGoal = 0;
  orderedBestNodes.push(startNode);

  while(!orderedBestNodes.empty()) {
  
    auto currentBestNode = orderedBestNodes.top();

    orderedBestNodes.pop();

    spdlog::debug("Current best node at location: [{0},{1}]. score: {2}", currentBestNode->location.x, currentBestNode->location.y, currentBestNode->scoreFromStart);

    if(currentBestNode->location == endLocation) {
      return reconstructPath(currentBestNode);
    }

    for(auto& inputMapping : actionInputs_.inputMappings) {
      auto actionId = inputMapping.first;
      auto mapping = inputMapping.second;

      // TODO: calculate this with respect to action/object orientation
      auto vectorToDest = mapping.vectorToDest;
      auto nextLocation = currentBestNode->location + vectorToDest;

      if(nextLocation.y < 0 || nextLocation.y >= grid_->getHeight() || nextLocation.x < 0 || nextLocation.x >= grid_->getWidth()) {
        continue;
      }

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
        std::shared_ptr<AStarPathNode> neighbourNode;

        if(nodes.find(nextLocation) != nodes.end()) {
          neighbourNode = nodes.at(nextLocation);
        } else {
          neighbourNode = std::make_shared<AStarPathNode>(AStarPathNode(nextLocation));
          nodes[nextLocation] = neighbourNode;
        }

        auto nextScoreToGoal = currentBestNode->scoreToGoal + glm::length((glm::vec2)mapping.vectorToDest);

        if(nextScoreToGoal < neighbourNode->scoreToGoal) {
          // We have found a better path
          

          // Set the action from the current best node to this node, and the parent of the neighbour to this node
          currentBestNode->actionId = actionId;
          neighbourNode->parent = currentBestNode;

          // Calculate the scores
          neighbourNode->scoreToGoal = nextScoreToGoal;
          neighbourNode->scoreFromStart = nextScoreToGoal + glm::distance((glm::vec2)endLocation, (glm::vec2)nextLocation);

          spdlog::debug("New scores for location: [{0},{1}], scoreToGoal: {2}, scoreFromStart: {3}", nextLocation.x, nextLocation.y, neighbourNode->scoreToGoal, neighbourNode->scoreFromStart);
          orderedBestNodes.push(neighbourNode);
          
        }
      }

    }
  }

  return SearchOutput();

}


}  // namespace griddly