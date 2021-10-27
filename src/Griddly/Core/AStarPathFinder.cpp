#include "AStarPathFinder.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <unordered_map>

#include "Grid.hpp"
#include "GDY/Objects/Object.hpp"
#include "GDY/Actions/Action.hpp"

namespace griddly {

AStarPathFinder::AStarPathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> impassableObjects, ActionInputsDefinition actionInputs)
    : PathFinder(grid, impassableObjects), actionInputs_(actionInputs) {
}

SearchOutput AStarPathFinder::reconstructPath(std::shared_ptr<AStarPathNode> currentBestNode) {
  if(currentBestNode->parent->parent == nullptr) {
    return {currentBestNode->actionId};
  } else {
    spdlog::debug("Reconstructing path: [{0},{1}]->[{2},{3}] actionId: {4}", currentBestNode->parent->location.x, currentBestNode->parent->location.y, currentBestNode->location.x, currentBestNode->location.y, currentBestNode->parent->actionId);
    return reconstructPath(currentBestNode->parent);
  }
  return {0};
}

SearchOutput AStarPathFinder::search(glm::ivec2 startLocation, glm::ivec2 endLocation, glm::ivec2 startOrientationVector, uint32_t maxDepth) {

  std::priority_queue<std::shared_ptr<AStarPathNode>, std::vector<std::shared_ptr<AStarPathNode>>, SortAStarPathNodes> orderedBestNodes;
  std::unordered_map<glm::ivec4, std::shared_ptr<AStarPathNode>> nodes;

  auto startNode = std::make_shared<AStarPathNode>(AStarPathNode(startLocation, startOrientationVector));
  startNode->scoreFromStart = glm::distance((glm::vec2)endLocation, (glm::vec2)startLocation);
  startNode->scoreToGoal = 0;
  orderedBestNodes.push(startNode);

  while(!orderedBestNodes.empty()) {
  
    auto currentBestNode = orderedBestNodes.top();

    orderedBestNodes.pop();

    spdlog::debug("Current best node at location: [{0},{1}]. score: {2}, action: {3}", currentBestNode->location.x, currentBestNode->location.y, currentBestNode->scoreFromStart, currentBestNode->actionId);

    if(currentBestNode->location == endLocation) {
      return reconstructPath(currentBestNode);
    }

    auto rotationMatrix = DiscreteOrientation(currentBestNode->orientationVector).getRotationMatrix();

    for(auto& inputMapping : actionInputs_.inputMappings) {
      auto actionId = inputMapping.first;
      auto mapping = inputMapping.second;

      auto vectorToDest = actionInputs_.relative ? mapping.vectorToDest * rotationMatrix:  mapping.vectorToDest;
      auto nextLocation = currentBestNode->location + vectorToDest;
      auto nextOrientation = actionInputs_.relative ? mapping.orientationVector * rotationMatrix:  mapping.orientationVector;

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

        auto nodeKey = glm::ivec4(nextLocation, nextOrientation);

        if(nodes.find(nodeKey) != nodes.end()) {
          neighbourNode = nodes.at(nodeKey);
        } else {
          neighbourNode = std::make_shared<AStarPathNode>(AStarPathNode(nextLocation, nextOrientation));
          nodes[nodeKey] = neighbourNode;
        }

        auto nextScoreToGoal = currentBestNode->scoreToGoal + glm::length((glm::vec2)mapping.vectorToDest);

        if(nextScoreToGoal < neighbourNode->scoreToGoal) {
          // We have found a better path

          // Set the action from the current best node to this node
          neighbourNode->actionId = actionId;
          neighbourNode->parent = currentBestNode;

          // Calculate the scores
          neighbourNode->scoreToGoal = nextScoreToGoal;
          neighbourNode->scoreFromStart = nextScoreToGoal + glm::distance((glm::vec2)endLocation, (glm::vec2)nextLocation);

          spdlog::debug("New scores for location: [{0},{1}], scoreToGoal: {2}, scoreFromStart: {3}, action: {4}", nextLocation.x, nextLocation.y, neighbourNode->scoreToGoal, neighbourNode->scoreFromStart, actionId);
          orderedBestNodes.push(neighbourNode);
          
        }
      }

    }
  }

  return SearchOutput();

}


}  // namespace griddly