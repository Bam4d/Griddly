#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "AStarPathFinder.hpp"
#include "GDY/Actions/Action.hpp"
#include "GDY/Objects/ObjectRules.hpp"
#include "Grid.hpp"

namespace griddly {

AStarPathFinder::AStarPathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects, ActionInputsDefinition actionInputs, PathFinderMode mode)
    : PathFinder(std::move(grid), std::move(impassableObjects), std::move(mode)), actionInputs_(std::move(std::move(actionInputs))) {
}

SearchOutput AStarPathFinder::reconstructPath(const std::shared_ptr<AStarPathNode>& currentBestNode) {
  // Walk back to the node right after the start; its actionId is the first step of the path.
  auto node = currentBestNode;
  while (node->parent != nullptr && node->parent->parent != nullptr) {
    node = node->parent;
  }
  return {node->parent == nullptr ? 0u : node->actionId};
}

SearchOutput AStarPathFinder::search(glm::ivec2 startLocation, glm::ivec2 endLocation, glm::ivec2 startOrientationVector, uint32_t maxDepth) {
  // A* over (location, orientation) states using the action's input mappings as edges.
  //
  // SEEK: shortest path to endLocation; returns the first action of it (0 if unreachable or already
  // there). FLEE: maximises the straight-line distance from endLocation within the explored region.
  // maxDepth bounds the number of node expansions; if the goal is not reached within the budget the
  // first action towards the most promising explored node is returned.
  //
  // The goal cell is always enterable (the target object itself may be listed as impassable, e.g. a
  // spider chasing a catcher it cannot walk through), other impassable objects block.
  std::priority_queue<std::shared_ptr<AStarPathNode>, std::vector<std::shared_ptr<AStarPathNode>>, SortAStarPathNodes> openSet;
  std::unordered_map<glm::ivec4, std::shared_ptr<AStarPathNode>> nodes;

  const bool flee = mode_ == PathFinderMode::FLEE;
  auto heuristic = [&](glm::ivec2 location) {
    const auto d = glm::distance(static_cast<glm::vec2>(endLocation), static_cast<glm::vec2>(location));
    return flee ? -d : d;
  };

  auto startNode = std::make_shared<AStarPathNode>(AStarPathNode(startLocation, startOrientationVector));
  startNode->scoreToGoal = 0;  // cost from the start so far (g)
  startNode->scoreFromStart = heuristic(startLocation);  // estimated total (f = g + h)
  nodes[glm::ivec4(startLocation, startOrientationVector)] = startNode;
  openSet.push(startNode);

  if (!flee && startLocation == endLocation) {
    return {0};
  }

  std::unordered_set<glm::ivec4> closed;
  // Fallback when the goal is not reached:
  //  - budget exhausted (goal may still be reachable): SEEK heads for the explored state with the best
  //    estimated total f = g + h (the most promising path so far);
  //  - search space exhausted (goal unreachable): SEEK heads for the explored state closest to the goal
  //    (fewest actions on ties) and stays put once there;
  //  - FLEE always heads for the explored state furthest from the target.
  std::shared_ptr<AStarPathNode> bestByF = startNode;
  std::shared_ptr<AStarPathNode> bestByH = startNode;
  auto dist = [&](const std::shared_ptr<AStarPathNode>& n) {
    return glm::distance(static_cast<glm::vec2>(endLocation), static_cast<glm::vec2>(n->location));
  };
  auto betterF = [&](const std::shared_ptr<AStarPathNode>& a, const std::shared_ptr<AStarPathNode>& b) {
    if (a->scoreFromStart != b->scoreFromStart) return a->scoreFromStart < b->scoreFromStart;
    return a->scoreToGoal > b->scoreToGoal;
  };
  auto betterH = [&](const std::shared_ptr<AStarPathNode>& a, const std::shared_ptr<AStarPathNode>& b) {
    const auto da = dist(a), db = dist(b);
    if (da != db) return flee ? da > db : da < db;
    return a->scoreToGoal < b->scoreToGoal;
  };

  const uint32_t budget = std::max<uint32_t>(maxDepth, 1);
  uint32_t expansions = 0;
  bool budgetExhausted = false;

  while (!openSet.empty()) {
    auto current = openSet.top();
    openSet.pop();

    const auto currentKey = glm::ivec4(current->location, current->orientationVector);
    if (closed.count(currentKey)) {
      continue;  // stale queue entry (a cheaper path to this state was found later)
    }
    closed.insert(currentKey);

    if (!flee && current->location == endLocation) {
      return reconstructPath(current);
    }
    if (current != startNode) {
      if (bestByF == startNode || betterF(current, bestByF)) bestByF = current;
    }
    if (betterH(current, bestByH)) bestByH = current;
    if (++expansions > budget) {
      budgetExhausted = true;
      break;
    }

    auto rotationMatrix = DiscreteOrientation(current->orientationVector).getRotationMatrix();

    for (const auto& inputMapping : actionInputs_.inputMappings) {
      const auto& actionId = inputMapping.first;
      const auto& mapping = inputMapping.second;

      const auto vectorToDest = actionInputs_.relative ? mapping.vectorToDest * rotationMatrix : mapping.vectorToDest;
      const auto nextLocation = current->location + vectorToDest;
      const auto nextOrientation = actionInputs_.relative ? mapping.orientationVector * rotationMatrix : mapping.orientationVector;

      if (nextLocation.y < 0 || nextLocation.y >= static_cast<int32_t>(grid_->getHeight()) || nextLocation.x < 0 || nextLocation.x >= static_cast<int32_t>(grid_->getWidth())) {
        continue;
      }

      if (nextLocation != endLocation) {
        bool passable = true;
        for (const auto& object : grid_->getObjectsAt(nextLocation)) {
          if (impassableObjects_.find(object.second->getObjectName()) != impassableObjects_.end()) {
            passable = false;
            break;
          }
        }
        if (!passable) {
          continue;
        }
      }

      const auto nodeKey = glm::ivec4(nextLocation, nextOrientation);
      if (closed.count(nodeKey)) {
        continue;
      }

      // every action costs 1 (rotations included), so A* minimises the number of actions
      const auto nextCost = current->scoreToGoal + 1.0f;

      auto it = nodes.find(nodeKey);
      std::shared_ptr<AStarPathNode> neighbourNode;
      if (it == nodes.end()) {
        neighbourNode = std::make_shared<AStarPathNode>(AStarPathNode(nextLocation, nextOrientation));
        nodes[nodeKey] = neighbourNode;
      } else {
        neighbourNode = it->second;
      }

      if (nextCost < neighbourNode->scoreToGoal) {
        neighbourNode->actionId = actionId;
        neighbourNode->parent = current;
        neighbourNode->scoreToGoal = nextCost;
        neighbourNode->scoreFromStart = nextCost + heuristic(nextLocation);
        spdlog::trace("A* node [{0},{1}] g={2} f={3} via action {4}", nextLocation.x, nextLocation.y, nextCost, neighbourNode->scoreFromStart, actionId);
        openSet.push(neighbourNode);
      }
    }
  }

  const auto& bestNode = (!flee && budgetExhausted) ? bestByF : bestByH;
  if (bestNode == startNode) {
    return {0};  // already as close as possible (SEEK) / as far as possible (FLEE)
  }
  return reconstructPath(bestNode);
}

}  // namespace griddly