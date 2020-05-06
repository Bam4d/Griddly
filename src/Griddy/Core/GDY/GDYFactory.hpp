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
  GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator);
  ~GDYFactory();

  static ActionBehaviourDefinition makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                    std::string objectName,
                                                    std::string associatedObjectName,
                                                    std::string actionName,
                                                    std::string commandName,
                                                    std::vector<std::string> commandParameters,
                                                    std::unordered_map<std::string, std::vector<std::string>> conditionalCommands);

  void createLevel(uint32_t width, uint32_t height, std::shared_ptr<Grid> grid);

  void loadLevel(uint32_t level);

  void initializeFromFile(std::string filename);

  void parseFromStream(std::istream& stream);

  void loadEnvironment(YAML::Node environment);
  void loadObjects(YAML::Node objects);
  void loadActions(YAML::Node actions);

  std::shared_ptr<LevelGenerator> getLevelGenerator() const;
  std::shared_ptr<ObjectGenerator> getObjectGenerator() const;
  std::unordered_map<std::string, std::string> getSpriteObserverDefinitions() const;
  std::unordered_map<std::string, BlockDefinition> getBlockObserverDefinitions() const;

  uint32_t getTileSize() const;
  std::string getName() const;
  uint32_t getNumLevels() const;

 private:

  void parseActionBehaviours(ActionBehaviourType actionBehaviourType, std::string objectName, std::string actionName, std::vector<std::string> associatedObjectNames, YAML::Node commands);

  std::vector<std::string> singleOrListNodeToList(YAML::Node singleOrList);

  BlockDefinition parseBlockObserverDefinition(YAML::Node blockNode);

  std::unordered_map<std::string, BlockDefinition> blockObserverDefinitions_;
  std::unordered_map<std::string, std::string> spriteObserverDefinitions_;

  uint32_t tileSize_ = 10;
  std::string name_;

  std::shared_ptr<MapReader> mapReaderLevelGenerator_;
  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  std::vector<std::string> levelStrings_;
};
}  // namespace griddy