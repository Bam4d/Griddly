#pragma once
#include <memory>
#include <sstream>

#include "../LevelGenerators/MapReader.hpp"
#include "../Observers/BlockObserver.hpp"
#include "../Observers/SpriteObserver.hpp"
#include "Objects/ObjectGenerator.hpp"
#include "TerminationGenerator.hpp"

namespace YAML {
class Node;
}

namespace griddly {

enum class PlayerMode {
  SINGLE,
  MULTI
};

enum class ActionControlMode {
  DIRECT,    // Control a single avatar directly
  SELECTION  // Select avatar by grid position as part of action
};

class GDYFactory {
 public:
  GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator);
  ~GDYFactory();

  static ActionBehaviourDefinition makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                           std::string objectName,
                                                           std::string associatedObjectName,
                                                           std::string actionName,
                                                           std::string commandName,
                                                           std::vector<std::string> commandParameters,
                                                           std::vector<std::unordered_map<std::string, std::vector<std::string>>> actionPreconditions,
                                                           std::unordered_map<std::string, std::vector<std::string>> conditionalCommands);

  void createLevel(uint32_t width, uint32_t height, std::shared_ptr<Grid> grid);

  void loadLevel(uint32_t level);

  void loadLevelString(std::string levelString);

  void initializeFromFile(std::string filename);

  void parseFromStream(std::istream& stream);

  void loadEnvironment(YAML::Node environment);
  void loadObjects(YAML::Node objects);
  void loadActions(YAML::Node actions);

  std::shared_ptr<TerminationGenerator> getTerminationGenerator() const;
  std::shared_ptr<LevelGenerator> getLevelGenerator() const;
  std::shared_ptr<ObjectGenerator> getObjectGenerator() const;
  std::unordered_map<std::string, SpriteDefinition> getSpriteObserverDefinitions() const;
  std::unordered_map<std::string, BlockDefinition> getBlockObserverDefinitions() const;

  std::unordered_map<std::string, int32_t> getGlobalParameterDefinitions() const;

  std::shared_ptr<TerminationHandler> createTerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) const;

  uint32_t getTileSize() const;
  std::string getName() const;
  uint32_t getNumLevels() const;

  ActionControlMode getActionControlMode() const;
  PlayerMode getPlayerMode() const;

 private:
  void parseActionBehaviours(
      ActionBehaviourType actionBehaviourType,
      std::string objectName,
      std::string actionName,
      std::vector<std::string> associatedObjectNames,
      YAML::Node commandsNode,
      YAML::Node preconditionsNode);

  std::vector<std::string> singleOrListNodeToList(YAML::Node singleOrList);

  void parseGlobalParameters(YAML::Node parametersNode);
  void parseTerminationConditions(YAML::Node terminationNode);
  void parseBlockObserverDefinition(std::string objectName, YAML::Node blockNode);
  void parseSpriteObserverDefinition(std::string objectName, YAML::Node spriteNode);
  void parsePlayerDefinition(YAML::Node playerNode);

  std::unordered_map<std::string, BlockDefinition> blockObserverDefinitions_;
  std::unordered_map<std::string, SpriteDefinition> spriteObserverDefinitions_;

  std::unordered_map<std::string, int32_t> globalParameterDefinitions_;

  uint32_t tileSize_ = 10;
  std::string name_ = "UnknownEnvironment";
  PlayerMode playerMode_;
  ActionControlMode actionControlMode_;

  std::shared_ptr<MapReader> mapReaderLevelGenerator_;
  const std::shared_ptr<ObjectGenerator> objectGenerator_;
  const std::shared_ptr<TerminationGenerator> terminationGenerator_;

  std::vector<std::string> levelStrings_;

};
}  // namespace griddly