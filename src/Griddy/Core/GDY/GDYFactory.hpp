#pragma once
#include <memory>
#include <sstream>

#include "../GameProcess.hpp"
#include "Objects/ObjectGenerator.hpp"
#include "../LevelGenerators/MapReader.hpp"
#include "../Observers/BlockObserver.hpp"

namespace YAML {
class Node;
}

namespace griddy {
class GDYFactory {
 public:
  GDYFactory();
  ~GDYFactory();

  void createLevel(uint width, uint height, std::shared_ptr<Grid>& grid);

  void loadLevel(uint level);

  void initializeFromFile(std::string filename);

  void parseFromStream(std::istream& stream);

  std::shared_ptr<LevelGenerator> getLevelGenerator() const;
  std::shared_ptr<ObjectGenerator> getObjectGenerator() const;
  std::unordered_map<std::string, std::string> getSpriteObserverDefinitions() const;
  std::unordered_map<std::string, BlockDefinition> getBlockObserverDefinitions() const;

  uint getTileSize() const;

 private:

  void loadEnvironment(YAML::Node environment);

  void loadObjects(YAML::Node objects);

  void loadActions(YAML::Node actions);

  void parseActionBehaviours(ActionBehaviourType actionBehaviourType, std::string objectName, std::string actionName, std::vector<std::string> associatedObjectNames, YAML::Node commands);

  std::vector<std::string> singleOrListNodeToList(YAML::Node singleOrList);

  ActionBehaviourDefinition makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                    std::string objectName,
                                                    std::string associatedObjectName,
                                                    std::string actionName,
                                                    std::string commandName,
                                                    std::vector<std::string> commandParameters,
                                                    std::unordered_map<std::string, std::vector<std::string>> conditionalCommands);

  BlockDefinition parseBlockObserverDefinition(YAML::Node blockNode);

  std::unordered_map<std::string, BlockDefinition> blockObserverDefinitions_;
  std::unordered_map<std::string, std::string> spriteObserverDefinitions_;

  uint tileSize_ = 10;

  std::shared_ptr<MapReader> mapReaderLevelGenerator_;
  std::shared_ptr<ObjectGenerator> objectGenerator_;

  std::vector<std::string> levelStrings_;
};
}  // namespace griddy