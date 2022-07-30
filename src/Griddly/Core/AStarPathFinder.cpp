#include <spdlog/spdlog.h>

#include <memory>
#include <unordered_map>
#include <utility>

#include "AStarPathFinder.hpp"
#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/Object.hpp"
#include "Grid.hpp"

namespace griddly {

AStarPathFinder::AStarPathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects, ActionInputsDefinition actionInputs, PathFinderMode mode)
    : PathFinder(std::move(grid), std::move(impassableObjects), std::move(mode)), actionInputs_(std::move(std::move(actionInputs))) {
}

SearchOutput AStarPathFinder::reconstructPath(const std::shared_ptr<AStarPathNode>& currentBestNode) {
  if (currentBestNode->parent == nullptr || currentBestNode->parent->parent == nullptr) {
    return {currentBestNode->actionId};
  }
  spdlog::debug("Reconstructing path: [{0},{1}]->[{2},{3}] actionId: {4}", currentBestNode->parent->location.x, currentBestNode->parent->location.y, currentBestNode->location.x, currentBestNode->location.y, currentBestNode->parent->actionId);
  return reconstructPath(currentBestNode->parent);

  return {0};
}

SearchOutput AStarPathFinder::search(glm::ivec2 startLocation, glm::ivec2 endLocation, glm::ivec2 startOrientationVector, uint32_t maxDepth) {
  std::priority_queue<std::shared_ptr<AStarPathNode>, std::vector<std::shared_ptr<AStarPathNode>>, SortAStarPathNodes> orderedBestNodes;
  std::unordered_map<glm::ivec4, std::shared_ptr<AStarPathNode>> nodes;

  const float behaviourCoeff = mode_ == PathFinderMode::FLEE ? -1. : 1.;

  auto startNode = std::make_shared<AStarPathNode>(AStarPathNode(startLocation, startOrientationVector));
  startNode->scoreFromStart = behaviourCoeff * glm::distance(static_cast<glm::vec2>(endLocation), static_cast<glm::vec2>(startLocation));
  startNode->scoreToGoal = 0;
  orderedBestNodes.push(startNode);

  uint32_t steps = 0;

  auto currentBestNode = orderedBestNodes.top();

  while (!orderedBestNodes.empty()) {
    orderedBestNodes.pop();

    spdlog::trace("Current best node at location: [{0},{1}]. score: {2}, action: {3}", currentBestNode->location.x, currentBestNode->location.y, currentBestNode->scoreFromStart, currentBestNode->actionId);

    if (currentBestNode->location == endLocation || steps >= maxDepth) {
      return reconstructPath(currentBestNode);
    }

    auto rotationMatrix = DiscreteOrientation(currentBestNode->orientationVector).getRotationMatrix();

    for (const auto& inputMapping : actionInputs_.inputMappings) {
      const auto& actionId = inputMapping.first;
      const auto& mapping = inputMapping.second;

      const auto& vectorToDest = actionInputs_.relative ? mapping.vectorToDest * rotationMatrix : mapping.vectorToDest;
      const auto& nextLocation = currentBestNode->location + vectorToDest;
      const auto& nextOrientation = actionInputs_.relative ? mapping.orientationVector * rotationMatrix : mapping.orientationVector;

      if (nextLocation.y < 0 || nextLocation.y >= grid_->getHeight() || nextLocation.x < 0 || nextLocation.x >= grid_->getWidth()) {
        continue;
      }

      // If this location is passable
      const auto& objectsAtNextLocation = grid_->getObjectsAt(nextLocation);
      bool passable = true;
      for (const auto& object : objectsAtNextLocation) {
        const auto& objectName = object.second->getObjectName();
        if (impassableObjects_.find(objectName) != impassableObjects_.end()) {
          passable = false;
          break;
        }
      }

      if (passable) {
        std::shared_ptr<AStarPathNode> neighbourNode;

        const auto nodeKey = glm::ivec4(nextLocation, nextOrientation);

        if (nodes.find(nodeKey) != nodes.end()) {
          neighbourNode = nodes.at(nodeKey);
        } else {
          neighbourNode = std::make_shared<AStarPathNode>(AStarPathNode(nextLocation, nextOrientation));
          nodes[nodeKey] = neighbourNode;
        }

        auto nextScoreToGoal = currentBestNode->scoreToGoal + glm::length(static_cast<glm::vec2>(mapping.vectorToDest));

        if (nextScoreToGoal < neighbourNode->scoreToGoal) {
          // We have found a better path

          // Set the action from the current best node to this node
          neighbourNode->actionId = actionId;
          neighbourNode->parent = currentBestNode;
          currentBestNode->child = neighbourNode;

          // Calculate the scores
          neighbourNode->scoreToGoal = nextScoreToGoal;
          neighbourNode->scoreFromStart = nextScoreToGoal + behaviourCoeff * glm::distance(static_cast<glm::vec2>(endLocation), static_cast<glm::vec2>(nextLocation));

          steps++;
          spdlog::trace("New scores for location: [{0},{1}], scoreToGoal: {2}, scoreFromStart: {3}, action: {4}. Steps: {5}", nextLocation.x, nextLocation.y, neighbourNode->scoreToGoal, neighbourNode->scoreFromStart, actionId, steps);
          orderedBestNodes.push(neighbourNode);
        }
      }
    }

    currentBestNode = orderedBestNodes.top();
  }

  return reconstructPath(currentBestNode);
}

}  // namespace griddly