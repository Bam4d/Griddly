#pragma once
#include <memory>
#include <sstream>

#include "../LevelGenerators/MapGenerator.hpp"
#include "../Observers/ASCIIObserver.hpp"
#include "../Observers/EntityObserver.hpp"

#ifndef WASM
#include "../Observers/BlockObserver.hpp"
#include "../Observers/IsometricSpriteObserver.hpp"
#include "../Observers/SpriteObserver.hpp"
#endif

#include "../Observers/NoneObserver.hpp"
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
  GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator);
  virtual ~GDYFactory() = default;

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

  virtual std::shared_ptr<Observer> createObserver(std::shared_ptr<Grid> grid, std::string observerName, uint32_t playerCount, uint32_t playerId = 0);
#ifndef WASM
  virtual std::unordered_map<std::string, SpriteDefinition> getIsometricSpriteObserverDefinitions() const;
  virtual std::unordered_map<std::string, SpriteDefinition> getSpriteObserverDefinitions() const;
  virtual std::unordered_map<std::string, BlockDefinition> getBlockObserverDefinitions() const;

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
  virtual std::string getAvatarObject() const;

  virtual YAML::iterator validateCommandPairNode(YAML::Node commandPairNodeList) const;

  virtual DefaultObserverConfig getDefaultObserverConfig() const;

  template <class ObserverConfigType>
  ObserverConfigType generateConfigForObserver(std::string observerName, bool isGlobalObserver = false);

  virtual ObserverType& getNamedObserverType(std::string observerName);

 private:
  void parseActionBehaviours(
      ActionBehaviourType actionBehaviourType,
      std::string objectName,
      std::string actionName,
      std::vector<std::string> associatedObjectNames,
      YAML::Node commandsNode,
      YAML::Node preconditionsNode);

  void parseGlobalVariables(YAML::Node variablesNode);

  bool parseTerminationConditionV2(TerminationState state, YAML::Node conditionNode);
  void parseTerminationConditionV1(TerminationState state, YAML::Node conditionNode);

  void parseTerminationConditions(YAML::Node terminationNode);
#ifndef WASM
  void parseShaderVariableConfig(YAML::Node shaderConfigNode);

  glm::uvec2 parseTileSize(YAML::Node observerConfigNode);

  void parseObjectBlockObserverDefinitions(BlockObserverConfig& observerConfig, std::unordered_map<std::string, YAML::Node> objectObserverConfigNodes);
  void parseObjectSpriteObserverDefinitions(SpriteObserverConfig& observerConfig, std::unordered_map<std::string, YAML::Node> objectObserverConfigNodes);
  void parseObjectIsometricObserverDefinitions(IsometricSpriteObserverConfig& observerConfig, std::unordered_map<std::string, YAML::Node> objectObserverConfigNodes);

  void parseObjectBlockObserverDefinition(BlockObserverConfig& observerConfig, std::string objectName, uint32_t renderTileId, YAML::Node blockNode);
  void parseObjectSpriteObserverDefinition(SpriteObserverConfig& observerConfig, std::string objectName, uint32_t renderTileId, YAML::Node spriteNode);
  void parseObjectIsometricObserverDefinition(IsometricSpriteObserverConfig& observerConfig, std::string objectName, uint32_t renderTileId, YAML::Node isometricSpriteNode);

#endif
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

  std::unordered_map<std::string, ObserverType> observerTypes_;

  const std::unordered_set<std::string> legacyNamedObservers_ = {
      "Vector", "Sprite2D", "Block2D", "Isometric", "ASCII", "Entity"};

  void registerObserverConfigNode(std::string observerName, YAML::Node observerConfigNode, bool useObserverNameAsType = false);

  template <class NodeValueType>
  NodeValueType resolveObserverConfigValue(std::string key, YAML::Node observerConfigNode, NodeValueType defaultValue, bool fallbackToDefaultConfig);

#ifndef WASM
  SpriteObserverConfig parseNamedSpriteObserverConfig(std::string observerName, bool isGlobalObserver);
  BlockObserverConfig parseNamedBlockObserverConfig(std::string observerName, bool isGlobalObserver);
  IsometricSpriteObserverConfig parseNamedIsometricObserverConfig(std::string observerName, bool isGlobalObserver);
#endif

  VectorObserverConfig parseNamedVectorObserverConfig(std::string observerName, bool isGlobalObserver);
  ASCIIObserverConfig parseNamedASCIIObserverConfig(std::string observerName, bool isGlobalObserver);
  EntityObserverConfig parseNamedEntityObserverConfig(std::string observerName, bool isGlobalObserver);

  void parseCommonObserverConfig(ObserverConfig& observerConfig, YAML::Node observerConfigNode, bool isGlobalObserver);

#ifndef WASM
  void parseNamedObserverShaderConfig(VulkanObserverConfig& config, YAML::Node observerConfigNode);
#endif

  const std::string& getPlayerObserverName() const;
  std::string playerObserverName_ = "";

  std::unordered_map<std::string, GlobalVariableDefinition> globalVariableDefinitions_;
  std::unordered_set<std::string> objectVariableNames_;  // Used for checking that object variables defined exist
  std::unordered_set<std::string> objectNames_;

  std::string name_ = "UnknownEnvironment";
  uint32_t playerCount_ = 0;
  std::string avatarObject_ = "";
  std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions_;
  std::unordered_map<std::string, ActionTriggerDefinition> actionTriggerDefinitions_;
  std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, float>>> behaviourProbabilities_;
  std::vector<std::string> externalActionNames_;

  std::vector<std::shared_ptr<MapGenerator>> mapLevelGenerators_;
  const std::shared_ptr<ObjectGenerator> objectGenerator_;
  const std::shared_ptr<TerminationGenerator> terminationGenerator_;

  YAML::Node defaultObserverConfigNode_;
  std::unordered_map<std::string, YAML::Node> observerConfigNodes_{};
  std::unordered_map<std::string, std::unordered_map<std::string, YAML::Node>> objectObserverConfigNodes_{};

  DefaultObserverConfig defaultObserverConfig_;
#ifndef WASM
  const ResourceConfig resourceConfig_;
#endif
};
}  // namespace griddly
