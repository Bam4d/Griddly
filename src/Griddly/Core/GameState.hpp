#pragma once

#include <map>
#include <string>
#include <vector>

#include "DelayedActionQueueItem.hpp"
#include "GDY/Actions/Direction.hpp"

namespace griddly {

class GameStateMapping {
 public:
  void addObject(const std::string& objectName) {
    // Set up default variables for all objects
    objectVariableNameToIdx[objectName]["_x"] = GameStateMapping::xIdx;
    objectVariableNameToIdx[objectName]["_y"] = GameStateMapping::yIdx;
    objectVariableNameToIdx[objectName]["_dx"] = GameStateMapping::dxIdx;
    objectVariableNameToIdx[objectName]["_dy"] = GameStateMapping::dyIdx;
    objectVariableNameToIdx[objectName]["_playerId"] = GameStateMapping::playerIdIdx;
    objectVariableNameToIdx[objectName]["_renderTileId"] = GameStateMapping::renderTileIdIdx;
  }

  const uint32_t addObjectVariable(const std::string& objectName, const std::string& variableName) {
    const uint32_t index = objectVariableNameToIdx.at(objectName).size();
    objectVariableNameToIdx.at(objectName).insert({variableName, index});
    return index;
  }

  // global variable name -> global variable value
  std::map<std::string, uint32_t> globalVariableNameToIdx;

  static const uint32_t xIdx = 0;
  static const uint32_t yIdx = 1;
  static const uint32_t dxIdx = 2;
  static const uint32_t dyIdx = 3;
  static const uint32_t playerIdIdx = 4;
  static const uint32_t renderTileIdIdx = 5;

  // object name -> variable name -> variable value
  std::map<std::string, std::map<std::string, uint32_t>> objectVariableNameToIdx;
};

class GameObjectData {
 public:
  uint32_t id;
  std::string name;
  std::vector<int32_t> variables;

  // Some helper methods for deserializing
  inline const std::map<std::string, uint32_t>& getVariableIndexes(const GameStateMapping& gameStateMapping) const {
    return gameStateMapping.objectVariableNameToIdx.at(name);
  }

  inline const int32_t getVariableValue(const std::map<std::string, uint32_t>& objectVariableIndexes, const std::string& variableName) const {
    return variables[objectVariableIndexes.at(variableName)];
  }

  inline const void setVariableValue(const std::map<std::string, uint32_t>& objectVariableIndexes, const std::string& variableName, int32_t value) {
    variables[objectVariableIndexes.at(variableName)] = value;
  }

  inline const glm::ivec2 getLocation(const std::map<std::string, uint32_t>& objectVariableIndexes) const {
    return {getVariableValue(objectVariableIndexes, "_x"), getVariableValue(objectVariableIndexes, "_y")};
  }

  inline const DiscreteOrientation getOrientation(const std::map<std::string, uint32_t>& objectVariableIndexes) const {
    return DiscreteOrientation(
        glm::ivec2(
            getVariableValue(objectVariableIndexes, "_dx"),
            getVariableValue(objectVariableIndexes, "_dy")));
  }
};

class GridState {
 public:
  uint32_t width;
  uint32_t height;
};

class DelayedActionData {
 public:
  uint32_t priority;
  uint32_t playerId;
  uint32_t sourceObjectIdx;
  std::string actionName;
  glm::ivec2 vectorToDest;
  glm::ivec2 orientationVector;
  uint32_t originatingPlayerId;
};

class SortDelayedActionData {
 public:
  bool operator()(const DelayedActionData& a, const DelayedActionData& b) {
    return a.priority > b.priority;
  };
};

class GameState {
 public:
  size_t hash = 0;
  uint32_t playerCount = 0;
  uint32_t tickCount = 0;
  GridState grid;
  std::vector<std::vector<int32_t>> globalData{};
  std::vector<GameObjectData> objectData{};
  VectorPriorityQueue<DelayedActionData, std::vector<DelayedActionData>, SortDelayedActionData> delayedActionData{};
};

}  // namespace griddly