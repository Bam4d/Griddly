#pragma once
#include <memory>
#include <sstream>

#include "../LevelGenerators/MapGenerator.hpp"
#include "../Observers/ASCIIObserver.hpp"
#include "../Observers/BlockObserver.hpp"
#include "../Observers/IsometricSpriteObserver.hpp"
#include "../Observers/SpriteObserver.hpp"
#include "../Observers/VectorObserver.hpp"
#include "../Players/Player.hpp"
#include "Objects/ObjectGenerator.hpp"
#include "TerminationGenerator.hpp"

namespace YAML {
class Node;
}

namespace griddly {

class GDYFactory {
 public:
  GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator, ResourceConfig resourceConfig);
  ~GDYFactory();

  static ActionBehaviourDefinition makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                           std::string objectName,
                                                           std::string associatedObjectName,
                                                           std::string actionName,
                                                           std::string commandName,
                                                           BehaviourCommandArguments commandArguments,
                                                           CommandList actionPreconditions,
                                                           CommandList conditionalCommands);

  void initializeFromFile(std::string filename);

  void parseFromStream(std::istream& stream);

  void loadEnvironment(YAML::Node environment);
  void loadObjects(YAML::Node objects);
  void loadActions(YAML::Node actions);

  virtual std::shared_ptr<TerminationGenerator> getTerminationGenerator() const;
  virtual std::shared_ptr<LevelGenerator> getLevelGenerator(uint32_t level) const;
  virtual std::shared_ptr<LevelGenerator> getLevelGenerator(std::string levelString) const;
  virtual std::shared_ptr<ObjectGenerator> getObjectGenerator() const;

  virtual std::shared_ptr<Observer> createObserver(std::shared_ptr<Grid> grid, ObserverType observerType) const;

  virtual std::unordered_map<std::string, SpriteDefinition> getIsometricSpriteObserverDefinitions() const;
  virtual std::unordered_map<std::string, SpriteDefinition> getSpriteObserverDefinitions() const;
  virtual std::unordered_map<std::string, BlockDefinition> getBlockObserverDefinitions() const;

  virtual ObserverConfig getSpriteObserverConfig() const;
  virtual ObserverConfig getIsometricSpriteObserverConfig() const;
  virtual ObserverConfig getBlockObserverConfig() const;
  virtual ObserverConfig getVectorObserverConfig() const;

  virtual std::unordered_map<std::string, GlobalVariableDefinition> getGlobalVariableDefinitions() const;

  virtual std::shared_ptr<TerminationHandler> createTerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) const;

  virtual void setMaxSteps(uint32_t maxSteps);
  virtual std::string getName() const;
  virtual uint32_t getLevelCount() const;
  virtual uint32_t getPlayerCount() const;

  virtual std::vector<std::string> getExternalActionNames() const;
  virtual std::unordered_map<std::string, ActionInputsDefinition> getActionInputsDefinitions() const;
  virtual std::unordered_map<std::string, ActionTriggerDefinition> getActionTriggerDefinitions() const;
  virtual ActionInputsDefinition findActionInputsDefinition(std::string actionName) const;
  virtual PlayerObserverDefinition getPlayerObserverDefinition() const;
  virtual std::string getAvatarObject() const;

  virtual YAML::iterator validateCommandPairNode(YAML::Node commandPairNodeList) const;

 private:
  void parseActionBehaviours(
      ActionBehaviourType actionBehaviourType,
      std::string objectName,
      std::string actionName,
      std::vector<std::string> associatedObjectNames,
      YAML::Node commandsNode,
      YAML::Node preconditionsNode);

  std::vector<std::string> singleOrListNodeToList(YAML::Node singleOrList);
  BehaviourCommandArguments singleOrListNodeToCommandArguments(YAML::Node singleOrList);

  void parseGlobalVariables(YAML::Node variablesNode);

  bool parseTerminationConditionV2(TerminationState state, YAML::Node conditionNode);
  void parseTerminationConditionV1(TerminationState state, YAML::Node conditionNode);

  void parseTerminationConditions(YAML::Node terminationNode);

  void parseIsometricSpriteObserverConfig(YAML::Node observerConfigNode);
  void parseSpriteObserverConfig(YAML::Node observerConfigNode);
  void parseBlockObserverConfig(YAML::Node observerConfigNode);
  void parseVectorObserverConfig(YAML::Node observerConfigNode);

  glm::ivec2 parseTileSize(YAML::Node observerConfigNode);

  void parseBlockObserverDefinitions(std::string objectName, YAML::Node blockNode);
  void parseBlockObserverDefinition(std::string objectName, uint32_t renderTileId, YAML::Node blockNode);
  void parseSpriteObserverDefinitions(std::string objectName, YAML::Node spriteNode);
  void parseSpriteObserverDefinition(std::string objectName, uint32_t renderTileId, YAML::Node spriteNode);
  void parseIsometricObserverDefinitions(std::string objectName, YAML::Node isometricNode);
  void parseIsometricObserverDefinition(std::string objectName, uint32_t renderTileId, YAML::Node isometricSpriteNode);
  void parsePlayerDefinition(YAML::Node playerNode);
  void parseCommandNode(
      std::string commandName,
      YAML::Node commandNode,
      ActionBehaviourType actionBehaviourType,
      std::string objectName,
      std::string actionName,
      std::vector<std::string> associatedObjectNames,
      CommandList actionPreconditions);

  std::unordered_map<uint32_t, InputMapping> defaultActionInputMappings() const;
  bool loadActionTriggerDefinition(std::unordered_set<std::string> sourceObjectNames, std::unordered_set<std::string> destinationObjectNames, std::string actionName, YAML::Node triggerNode);
  void loadActionInputsDefinition(std::string actionName, YAML::Node actionInputMappingNode);

  std::unordered_map<std::string, BlockDefinition> blockObserverDefinitions_;
  std::unordered_map<std::string, SpriteDefinition> spriteObserverDefinitions_;
  std::unordered_map<std::string, SpriteDefinition> isometricObserverDefinitions_;

  PlayerObserverDefinition playerObserverDefinition_{};

  ObserverConfig spriteObserverConfig_{};
  ObserverConfig isometricSpriteObserverConfig_{};
  ObserverConfig blockObserverConfig_{};
  ObserverConfig vectorObserverConfig_{};

  ResourceConfig resourceConfig_;

  std::unordered_map<std::string, GlobalVariableDefinition> globalVariableDefinitions_;

  std::string name_ = "UnknownEnvironment";
  uint32_t playerCount_;
  std::string avatarObject_ = "";
  std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions_;
  std::unordered_map<std::string, ActionTriggerDefinition> actionTriggerDefinitions_;
  std::unordered_map<std::string, float> actionProbabilities_;
  std::vector<std::string> externalActionNames_;

  std::vector<std::shared_ptr<MapGenerator>> mapLevelGenerators_;
  const std::shared_ptr<ObjectGenerator> objectGenerator_;
  const std::shared_ptr<TerminationGenerator> terminationGenerator_;
};
}  // namespace griddly