#pragma once

#include <map>
#include <string>
#include <vector>

namespace griddly {

class GameStateMapping {
 public:
  // global variable name -> global variable value
  std::map<std::string, uint32_t> globalVariableNameToIdx;

  // object name -> variable name -> variable value
  std::map<std::string, std::map<std::string, uint32_t>> objectVariableNameToIdx;
};

class GameObjectData {
 public:
  uint32_t id;
  std::string name;
  std::vector<int32_t> variables;

  inline const std::map<std::string, int32_t>& getVariableIndexes(const GameStateMapping& gameStateMapping) {
    return gameStateMapping.objectVariableNameToIdx.at(name);
  }

  inline const int32_t getVariableValue(const std::map<std::string, int32_t>& objectVariableIndexes, const std::string& variableName) {
    return variables[objectVariableIndexes.at(variableName)];
  }

  inline const void setVariableValue(const std::map<std::string, int32_t>& objectVariableIndexes, const std::string& variableName, int32_t value) {
    variables[objectVariableIndexes.at(variableName)] = value;
  }
};

class GameState {
 public:
  uint32_t players;
  uint32_t tickCount;
  std::vector<int32_t> defaultEmptyObjectIdx;
  std::vector<int32_t> defaultBoundaryObjectIdx;
  std::vector<std::vector<int32_t>> globalData;
  std::vector<GameObjectData> objectData;

  
};

}  // namespace griddly