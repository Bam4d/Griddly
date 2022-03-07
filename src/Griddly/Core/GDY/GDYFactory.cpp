#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>
#include <utility>

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "../Grid.hpp"
#include "../TurnBasedGameProcess.hpp"
#include "GDYFactory.hpp"
#include "YAMLUtils.hpp"

#define EMPTY_NODE YAML::Node()

namespace griddly {

GDYFactory::GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator, ResourceConfig resourceConfig)
    : objectGenerator_(std::move(objectGenerator)),
      terminationGenerator_(std::move(terminationGenerator)),
      resourceConfig_(std::move(resourceConfig)) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
}

void GDYFactory::initializeFromFile(std::string filename) {
  spdlog::info("Loading GDY file: {0}", filename);
  std::ifstream gdyFile;
  gdyFile.open(filename);

  if (gdyFile.fail()) {
    auto error = fmt::format("Cannot find the file {0}", filename);
    spdlog::error(error);
    throw std::invalid_argument(error);
  }
  parseFromStream(gdyFile);
}

void GDYFactory::parseFromStream(std::istream& stream) {
  auto gdyConfig = YAML::Load(stream);

  auto versionNode = gdyConfig["Version"];
  auto version = versionNode.as<float>(0.1);
  spdlog::info("Loading GDY file Version: {0}.", version);

  auto environment = gdyConfig["Environment"];
  auto objects = gdyConfig["Objects"];
  auto actions = gdyConfig["Actions"];

  loadObjects(objects);
  loadActions(actions);

  if (version == 0.2f) {
    loadEnvironment02(environment);
  } else {
    loadEnvironment(environment);
  }
}

void GDYFactory::loadEnvironment(YAML::Node environment) {
  spdlog::info("Loading V0.1 Environment...");

  if (environment["Name"].IsDefined()) {
    name_ = environment["Name"].as<std::string>();
    spdlog::debug("Setting environment name: {0}", name_);
  }

  parsePlayerDefinition(environment["Player"]);

  auto observerConfigNode = environment["Observers"];

  registerObserverConfigNode("VECTOR", observerConfigNode["Vector"], true);
  registerObserverConfigNode("SPRITE_2D", observerConfigNode["Sprite2D"], true);
  registerObserverConfigNode("BLOCK_2D", observerConfigNode["Block2D"], true);
  registerObserverConfigNode("ISOMETRIC", observerConfigNode["Isometric"], true);
  registerObserverConfigNode("ASCII", observerConfigNode["ASCII"], true);

  observerTypes_.insert({"NONE", ObserverType::NONE});

  parseGlobalVariables(environment["Variables"]);
  parseTerminationConditions(environment["Termination"]);

  auto levels = environment["Levels"];
  for (auto&& level : levels) {
    auto levelStringStream = std::stringstream(level.as<std::string>());

    auto mapGenerator = std::make_shared<MapGenerator>(MapGenerator(playerCount_, objectGenerator_));
    mapGenerator->parseFromStream(levelStringStream);
    mapLevelGenerators_.push_back(mapGenerator);
  }

  spdlog::info("Loaded {0} levels", mapLevelGenerators_.size());
}

void GDYFactory::loadEnvironment02(YAML::Node environment) {
  spdlog::info("Loading V0.2 Environment...");

  if (environment["Name"].IsDefined()) {
    name_ = environment["Name"].as<std::string>();
    spdlog::debug("Setting environment name: {0}", name_);
  }

  parsePlayerDefinition(environment["Player"]);

  auto observerConfigNode = environment["Observers"];
  if (observerConfigNode.IsDefined()) {
    for (YAML::const_iterator namedObserverNode = observerConfigNode.begin(); namedObserverNode != observerConfigNode.end(); ++namedObserverNode) {
      auto observerName = namedObserverNode->first.as<std::string>();
      auto namedObserverConfigNode = namedObserverNode->second;

      if (legacyNamedObservers_.find(observerName) == legacyNamedObservers_.end()) {
        registerObserverConfigNode(observerName, namedObserverConfigNode);
      }
    }
  }

  registerObserverConfigNode("VECTOR", observerConfigNode["Vector"], true);
  registerObserverConfigNode("SPRITE_2D", observerConfigNode["Sprite2D"], true);
  registerObserverConfigNode("BLOCK_2D", observerConfigNode["Block2D"], true);
  registerObserverConfigNode("ISOMETRIC", observerConfigNode["Isometric"], true);
  registerObserverConfigNode("ASCII", observerConfigNode["ASCII"], true);
  observerTypes_.insert({"NONE", ObserverType::NONE});

  parseGlobalVariables(environment["Variables"]);
  parseTerminationConditions(environment["Termination"]);

  auto levels = environment["Levels"];
  for (std::size_t l = 0; l < levels.size(); l++) {
    auto levelStringStream = std::stringstream(levels[l].as<std::string>());

    auto mapGenerator = std::make_shared<MapGenerator>(MapGenerator(playerCount_, objectGenerator_));
    mapGenerator->parseFromStream(levelStringStream);
    mapLevelGenerators_.push_back(mapGenerator);
  }

  spdlog::info("Loaded {0} levels", mapLevelGenerators_.size());
}

void GDYFactory::registerObserverConfigNode(std::string observerName, YAML::Node observerConfigNode, bool useObserverNameAsType) {
  std::string observerTypeString;
  if (!useObserverNameAsType) {
    if (!observerConfigNode["Type"].IsDefined()) {
      auto error = fmt::format("Observers must have a ObserverType defined.");
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    observerTypeString = observerConfigNode["Type"].as<std::string>();
  } else {
    observerTypeString = observerName;
  }

  spdlog::debug("Parsing named observer config with observer name: {0} and type: {1}", observerName, observerTypeString);

  if (observerTypeString == "VECTOR") {
    observerTypes_.insert({observerName, ObserverType::VECTOR});
  } else if (observerTypeString == "SPRITE_2D") {
    observerTypes_.insert({observerName, ObserverType::SPRITE_2D});
  } else if (observerTypeString == "BLOCK_2D") {
    observerTypes_.insert({observerName, ObserverType::BLOCK_2D});
  } else if (observerTypeString == "ISOMETRIC") {
    observerTypes_.insert({observerName, ObserverType::ISOMETRIC});
  } else if (observerTypeString == "ASCII") {
    observerTypes_.insert({observerName, ObserverType::ASCII});
  }
  // else if (observerType == "ENTITY") {
  // }
  else {
    auto error = fmt::format("Unknown or undefined observer type: {0}", observerTypeString);
    spdlog::error(error);
    throw std::invalid_argument(error);
  }
  observerConfigNodes_.insert({observerName, observerConfigNode});
}

template <class ObserverConfigType>
ObserverConfigType GDYFactory::generateConfigForObserver(std::string observerName, bool isGlobalObserver) {
  std::shared_ptr<ObserverConfig> config;
  switch (observerTypes_.at(observerName)) {
    case ObserverType::VECTOR:
      config = std::make_shared<VectorObserverConfig>(parseNamedVectorObserverConfigV2(observerName, isGlobalObserver));
      break;
    case ObserverType::SPRITE_2D:
      config = std::make_shared<VulkanGridObserverConfig>(parseNamedSpriteObserverConfigV2(observerName, isGlobalObserver));
      break;
    case ObserverType::BLOCK_2D:
      config = std::make_shared<VulkanGridObserverConfig>(parseNamedBlockObserverConfigV2(observerName, isGlobalObserver));
      break;
    case ObserverType::ASCII:
      config = std::make_shared<ASCIIObserverConfig>(parseNamedASCIIObserverConfigV2(observerName, isGlobalObserver));
      break;
    case ObserverType::ISOMETRIC:
      config = std::make_shared<IsometricSpriteObserverConfig>(parseNamedIsometricObserverConfigV2(observerName, isGlobalObserver));
      break;
    case ObserverType::NONE:
      config = std::make_shared<ObserverConfig>();
      break;
    default: {
      auto error = fmt::format("Unknown or undefined observer: {0}", observerName);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }
  }

  return *std::static_pointer_cast<ObserverConfigType>(config);
}

template <class NodeValueType>
NodeValueType GDYFactory::resolveObserverConfigValue(std::string key, YAML::Node observerConfigNode, NodeValueType defaultValue, bool fallbackToDefaultConfig) {
  return observerConfigNode[key].as<NodeValueType>(fallbackToDefaultConfig ? defaultObserverConfigNode_[key].as<NodeValueType>(defaultValue) : defaultValue);
}

VectorObserverConfig GDYFactory::parseNamedVectorObserverConfigV2(std::string observerName, bool isGlobalObserver) {
  VectorObserverConfig config{};

  spdlog::debug("Parsing VECTOR observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);

  config.includePlayerId = resolveObserverConfigValue<bool>("IncludePlayerId", observerConfigNode, config.includePlayerId, !isGlobalObserver);
  config.includeRotation = resolveObserverConfigValue<bool>("IncludeRotation", observerConfigNode, config.includeRotation, !isGlobalObserver);
  config.includeVariables = resolveObserverConfigValue<bool>("IncludeVariables", observerConfigNode, config.includeVariables, !isGlobalObserver);

  return config;
}

VulkanGridObserverConfig GDYFactory::parseNamedSpriteObserverConfigV2(std::string observerName, bool isGlobalObserver) {
  VulkanGridObserverConfig config{};

  spdlog::debug("Parsing SPRITE observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);
  parseNamedObserverShaderConfig(config, observerConfigNode);

  config.tileSize = parseTileSize(observerConfigNode);
  config.highlightPlayers = resolveObserverConfigValue<bool>("HighlightPlayers", observerConfigNode, playerCount_ > 1, !isGlobalObserver);
  config.rotateAvatarImage = resolveObserverConfigValue<bool>("RotateAvatarImage", observerConfigNode, config.rotateAvatarImage, !isGlobalObserver);

  auto backgroundTileNode = observerConfigNode["BackgroundTile"];
  if (backgroundTileNode.IsDefined()) {
    auto backgroundTile = backgroundTileNode.as<std::string>();
    spdlog::debug("Setting background tiling to {0}", backgroundTile);
    SpriteDefinition backgroundTileDefinition{};
    backgroundTileDefinition.images = {backgroundTile};
    spriteObserverDefinitions_.insert({"_background_", backgroundTileDefinition});
  }

  return config;
}

VulkanGridObserverConfig GDYFactory::parseNamedBlockObserverConfigV2(std::string observerName, bool isGlobalObserver) {
  VulkanGridObserverConfig config{};

  spdlog::debug("Parsing BLOCK observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);
  parseNamedObserverShaderConfig(config, observerConfigNode);

  config.tileSize = parseTileSize(observerConfigNode);
  config.highlightPlayers = resolveObserverConfigValue<bool>("HighlightPlayers", observerConfigNode, playerCount_ > 1, !isGlobalObserver);
  config.rotateAvatarImage = resolveObserverConfigValue<bool>("RotateAvatarImage", observerConfigNode, config.rotateAvatarImage, !isGlobalObserver);

  return config;
}

ASCIIObserverConfig GDYFactory::parseNamedASCIIObserverConfigV2(std::string observerName, bool isGlobalObserver) {
  ASCIIObserverConfig config{};

  spdlog::debug("Parsing ASCII observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);

  config.asciiPadWidth = resolveObserverConfigValue<int32_t>("Padding", observerConfigNode, config.asciiPadWidth, !isGlobalObserver);
  config.includePlayerId = resolveObserverConfigValue<bool>("IncludePlayerId", observerConfigNode, config.includePlayerId, !isGlobalObserver);

  return config;
}

IsometricSpriteObserverConfig GDYFactory::parseNamedIsometricObserverConfigV2(std::string observerName, bool isGlobalObserver) {
  IsometricSpriteObserverConfig config{};

  spdlog::debug("Parsing ISOMETRIC observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);
  parseNamedObserverShaderConfig(config, observerConfigNode);

  config.tileSize = parseTileSize(observerConfigNode);
  config.isoTileDepth = resolveObserverConfigValue<int32_t>("IsoTileDepth", observerConfigNode, config.isoTileDepth, !isGlobalObserver);
  config.isoTileHeight = resolveObserverConfigValue<int32_t>("IsoTileHeight", observerConfigNode, config.isoTileHeight, !isGlobalObserver);
  config.highlightPlayers = resolveObserverConfigValue<bool>("HighlightPlayers", observerConfigNode, playerCount_ > 1, !isGlobalObserver);

  auto isometricBackgroundTileNode = observerConfigNode["BackgroundTile"];
  if (isometricBackgroundTileNode.IsDefined()) {
    auto backgroundTile = isometricBackgroundTileNode.as<std::string>();
    spdlog::debug("Setting background tiling to {0}", backgroundTile);
    SpriteDefinition backgroundTileDefinition{};
    backgroundTileDefinition.images = {backgroundTile};
    isometricObserverDefinitions_.insert({"_iso_background_", backgroundTileDefinition});
  }
  return config;
}

// void GDYFactory::parseNamedEntityObserverConfig(std::string observerName, YAML::Node observerConfigNode) {

void GDYFactory::parseCommonObserverConfig(ObserverConfig& observerConfig, YAML::Node observerConfigNode, bool isGlobalObserver) {
  spdlog::debug("Parsing common observer config...");
  observerConfig.overrideGridWidth = resolveObserverConfigValue<int32_t>("Width", observerConfigNode, observerConfig.overrideGridWidth, !isGlobalObserver);
  observerConfig.overrideGridHeight = resolveObserverConfigValue<int32_t>("Height", observerConfigNode, observerConfig.overrideGridHeight, !isGlobalObserver);
  observerConfig.gridXOffset = resolveObserverConfigValue<int32_t>("OffsetX", observerConfigNode, observerConfig.gridXOffset, !isGlobalObserver);
  observerConfig.gridYOffset = resolveObserverConfigValue<int32_t>("OffsetY", observerConfigNode, observerConfig.gridYOffset, !isGlobalObserver);
  observerConfig.trackAvatar = resolveObserverConfigValue<bool>("TrackAvatar", observerConfigNode, observerConfig.trackAvatar, !isGlobalObserver);
  observerConfig.rotateWithAvatar = resolveObserverConfigValue<bool>("RotateWithAvatar", observerConfigNode, observerConfig.rotateWithAvatar, !isGlobalObserver);
}

void GDYFactory::parseNamedObserverShaderConfig(VulkanObserverConfig& config, YAML::Node observerConfigNode) {
  auto shaderConfigNode = observerConfigNode["Shader"];
  if (!shaderConfigNode.IsDefined()) {
    spdlog::debug("Passing no additional variables to shaders");
    return;
  }

  auto globalVariableNode = shaderConfigNode["GlobalVariables"];
  if (globalVariableNode.IsDefined()) {
    for (std::size_t i = 0; i < globalVariableNode.size(); i++) {
      auto globalVariableName = globalVariableNode[i].as<std::string>();

      // Check the variable exists
      if (globalVariableDefinitions_.find(globalVariableName) == globalVariableDefinitions_.end()) {
        std::string error = fmt::format("No global variable with name {0} exists to expose to shaders", globalVariableName);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
      config.shaderVariableConfig.exposedGlobalVariables.push_back(globalVariableName);
    }
  }

  auto objectVariableNode = shaderConfigNode["ObjectVariables"];
  if (objectVariableNode.IsDefined()) {
    for (std::size_t i = 0; i < objectVariableNode.size(); i++) {
      auto objectVariableName = objectVariableNode[i].as<std::string>();

      // Check the variable exists
      if (objectVariableNames_.find(objectVariableName) == objectVariableNames_.end()) {
        std::string error = fmt::format("No object variable with name {0} exists to expose to shaders", objectVariableName);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
      config.shaderVariableConfig.exposedObjectVariables.push_back(objectVariableName);
    }
  }
}

glm::uvec2 GDYFactory::parseTileSize(YAML::Node observerConfigNode) {
  glm::uvec2 tileSize{24, 24};
  if (observerConfigNode["TileSize"].IsDefined()) {
    auto tileSizeNode = observerConfigNode["TileSize"];
    if (tileSizeNode.IsScalar()) {
      tileSize = glm::uvec2(tileSizeNode.as<uint32_t>());
    } else if (tileSizeNode.IsSequence()) {
      tileSize.x = tileSizeNode[0].as<uint32_t>();
      tileSize.y = tileSizeNode[1].as<uint32_t>();
    }
  }

  return tileSize;
}

void GDYFactory::parsePlayerDefinition(YAML::Node playerNode) {
  if (!playerNode.IsDefined()) {
    spdlog::debug("No player configuration node specified, assuming default action control.");
    playerCount_ = 1;
    return;
  }

  auto playerCountNode = playerNode["Count"];
  if (playerCountNode.IsDefined()) {
    playerCount_ = playerCountNode.as<uint32_t>();
  } else {
    playerCount_ = 1;
  }

  defaultObserverConfig_.playerCount = playerCount_;
  defaultObserverConfig_.highlightPlayers = playerCount_ > 1;

  // If all actions control a single avatar type
  auto avatarObjectNode = playerNode["AvatarObject"];
  if (avatarObjectNode.IsDefined()) {
    auto avatarObjectName = avatarObjectNode.as<std::string>();
    objectGenerator_->setAvatarObject(avatarObjectName);

    spdlog::debug("Actions will control the object with name={0}", avatarObjectName);

    avatarObject_ = avatarObjectName;

    // Parse default observer rules
    auto observerNode = playerNode["Observer"];
    if (observerNode.IsDefined()) {
      spdlog::debug("Parsing player observer definition");
      defaultObserverConfigNode_ = observerNode;
      auto observerGridWidth = observerNode["Width"].as<uint32_t>(0);
      auto observerGridHeight = observerNode["Height"].as<uint32_t>(0);
      auto observerGridOffsetX = observerNode["OffsetX"].as<int32_t>(0);
      auto observerGridOffsetY = observerNode["OffsetY"].as<int32_t>(0);
      auto trackAvatar = observerNode["TrackAvatar"].as<bool>(false);
      auto rotateWithAvatar = observerNode["RotateWithAvatar"].as<bool>(false);
      auto rotateAvatarImage = observerNode["RotateAvatarImage"].as<bool>(true);
      auto highlightPlayers = observerNode["HighlightPlayers"].as<bool>(playerCount_ > 1);

      if (highlightPlayers) {
        spdlog::debug("GDYFactory highlight players = True");
      }

      defaultObserverConfig_.overrideGridHeight = observerGridHeight;
      defaultObserverConfig_.overrideGridWidth = observerGridWidth;
      defaultObserverConfig_.gridXOffset = observerGridOffsetX;
      defaultObserverConfig_.gridYOffset = observerGridOffsetY;
      defaultObserverConfig_.trackAvatar = trackAvatar;
      defaultObserverConfig_.rotateAvatarImage = rotateAvatarImage;
      defaultObserverConfig_.rotateWithAvatar = rotateWithAvatar;
      defaultObserverConfig_.highlightPlayers = highlightPlayers;
    }
  }
}

YAML::iterator GDYFactory::validateCommandPairNode(YAML::Node commandPairNodeList) const {
  if (commandPairNodeList.size() > 1) {
    auto line = commandPairNodeList.Mark().line;
    auto errorString = fmt::format("Parse Error line {0}. Each command must be defined as a singleton list. E.g '- set: ...\n- reward: ...'. \n You may have a missing '-' before the command.", line);
    spdlog::error(errorString);
    throw std::invalid_argument(errorString);
  }

  return commandPairNodeList.begin();
}

void GDYFactory::parseTerminationConditionV1(TerminationState state, YAML::Node conditionNode) {
  for (auto&& c : conditionNode) {
    auto commandIt = validateCommandPairNode(c);
    auto commandName = commandIt->first.as<std::string>();
    auto commandArguments = singleOrListNodeToList(commandIt->second);

    terminationGenerator_->defineTerminationCondition(state, commandName, 0, 0, commandArguments);
  }
}

bool GDYFactory::parseTerminationConditionV2(TerminationState state, YAML::Node conditionListNode) {
  for (auto&& c : conditionListNode) {
    auto conditionNode = c["Conditions"];
    if (!conditionNode.IsDefined()) {
      return false;
    }

    auto rewardNode = c["Reward"];
    auto opposingRewardNode = c["OpposingReward"];

    auto reward = rewardNode.as<int32_t>(0);
    auto opposingReward = opposingRewardNode.as<int32_t>(0);

    for (auto&& i : conditionNode) {
      auto commandIt = validateCommandPairNode(i);
      auto commandName = commandIt->first.as<std::string>();
      auto commandArguments = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(state, commandName, reward, opposingReward, commandArguments);
    }
  }

  return true;
}

void GDYFactory::parseTerminationConditions(YAML::Node terminationNode) {
  if (!terminationNode.IsDefined()) {
    return;
  }

  auto winNode = terminationNode["Win"];
  if (winNode.IsDefined()) {
    spdlog::debug("Parsing win conditions");
    if (!parseTerminationConditionV2(TerminationState::WIN, winNode)) {
      parseTerminationConditionV1(TerminationState::WIN, winNode);
    }
  }

  auto loseNode = terminationNode["Lose"];
  if (loseNode.IsDefined()) {
    spdlog::debug("Parsing lose conditions.");
    if (!parseTerminationConditionV2(TerminationState::LOSE, loseNode)) {
      parseTerminationConditionV1(TerminationState::LOSE, loseNode);
    }
  }

  auto endNode = terminationNode["End"];
  if (endNode.IsDefined()) {
    spdlog::debug("Parsing end conditions.");
    if (!parseTerminationConditionV2(TerminationState::NONE, endNode)) {
      parseTerminationConditionV1(TerminationState::NONE, endNode);
    }
  }
}

void GDYFactory::setMaxSteps(uint32_t maxSteps) {
  terminationGenerator_->defineTerminationCondition(TerminationState::LOSE, "gt", 0, 0, {"_steps", std::to_string(maxSteps)});
}

void GDYFactory::parseGlobalVariables(YAML::Node variablesNode) {
  if (!variablesNode.IsDefined()) {
    return;
  }

  for (auto&& p : variablesNode) {
    auto variable = p;
    auto variableName = variable["Name"].as<std::string>();
    auto variableInitialValue = variable["InitialValue"].as<int32_t>(0);
    auto variablePerPlayer = variable["PerPlayer"].as<bool>(false);

    spdlog::debug("Parsed global variable {0} with value {1}", variableName, variableInitialValue);

    GlobalVariableDefinition globalVariableDefinition{
        variableInitialValue, variablePerPlayer};

    globalVariableDefinitions_.insert({variableName, globalVariableDefinition});
  }
}

void GDYFactory::loadObjects(YAML::Node objects) {
  spdlog::info("Loading {0} objects...", objects.size());

  for (auto&& i : objects) {
    auto object = i;
    auto objectName = object["Name"].as<std::string>();
    auto mapCharacter = object["MapCharacter"].as<char>('?');
    auto observerDefinitions = object["Observers"];

    if (observerDefinitions.IsDefined()) {
      parseSpriteObserverDefinitions(objectName, observerDefinitions["Sprite2D"]);
      parseBlockObserverDefinitions(objectName, observerDefinitions["Block2D"]);
      parseIsometricObserverDefinitions(objectName, observerDefinitions["Isometric"]);
    }

    auto variables = object["Variables"];
    std::unordered_map<std::string, uint32_t> variableDefinitions;

    if (variables.IsDefined()) {
      for (auto&& p : variables) {
        auto variable = p;
        auto variableName = variable["Name"].as<std::string>();
        auto variableInitialValue = variable["InitialValue"].as<uint32_t>(0);
        variableDefinitions.insert({variableName, variableInitialValue});
        objectVariableNames_.insert(variableName);
      }
    }

    uint32_t zIdx = 0;
    auto objectZIdx = object["Z"];
    if (objectZIdx.IsDefined()) {
      zIdx = objectZIdx.as<uint32_t>();
    }

    objectGenerator_->defineNewObject(objectName, mapCharacter, zIdx, variableDefinitions);

    auto initialActionsNode = object["InitialActions"];

    if (initialActionsNode.IsDefined()) {
      for (auto&& a : initialActionsNode) {
        auto initialActionNode = a;
        auto actionName = initialActionNode["Action"].as<std::string>();
        auto actionId = initialActionNode["ActionId"].as<uint32_t>(0);
        auto delay = initialActionNode["Delay"].as<uint32_t>(0);
        auto randomize = initialActionNode["Randomize"].as<bool>(false);

        objectGenerator_->addInitialAction(objectName, actionName, actionId, delay, randomize);
      }
    }
  }

  // Validate we have observer definitions for each objects
  if (spriteObserverDefinitions_.size() > 0) {
    for (auto&& i : objects) {
      auto object = i;
      auto objectName = object["Name"].as<std::string>();
      auto observerDefinitions = object["Observers"];
      if (!observerDefinitions["Sprite2D"].IsDefined()) {
        throw std::invalid_argument(fmt::format("Object {0} missing Sprite2D observer definition", objectName));
      }
    }
  }

  if (blockObserverDefinitions_.size() > 0) {
    for (auto&& i : objects) {
      auto object = i;
      auto objectName = object["Name"].as<std::string>();
      auto observerDefinitions = object["Observers"];
      if (!observerDefinitions["Block2D"].IsDefined()) {
        throw std::invalid_argument(fmt::format("Object {0} missing Block2D observer definition", objectName));
      }
    }
  }

  if (isometricObserverDefinitions_.size() > 0) {
    for (auto&& i : objects) {
      auto object = i;
      auto objectName = object["Name"].as<std::string>();
      auto observerDefinitions = object["Observers"];
      if (!observerDefinitions["Isometric"].IsDefined()) {
        throw std::invalid_argument(fmt::format("Object {0} missing Isometric observer definition", objectName));
      }
    }
  }
}

void GDYFactory::parseIsometricObserverDefinitions(std::string objectName, YAML::Node isometricObserverNode) {
  if (!isometricObserverNode.IsDefined()) {
    return;
  }

  if (isometricObserverNode.IsSequence()) {
    for (std::size_t c = 0; c < isometricObserverNode.size(); c++) {
      parseIsometricObserverDefinition(objectName, c, isometricObserverNode[c]);
    }
  } else {
    parseIsometricObserverDefinition(objectName, 0, isometricObserverNode);
  }
}

void GDYFactory::parseIsometricObserverDefinition(std::string objectName, uint32_t renderTileId, YAML::Node isometricSpriteNode) {
  SpriteDefinition spriteDefinition{};
  spriteDefinition.images = singleOrListNodeToList(isometricSpriteNode["Image"]);
  spriteDefinition.scale = isometricSpriteNode["Scale"].as<float>(1.0f);

  std::string renderTileName = objectName + std::to_string(renderTileId);

  auto tileOffsetNode = isometricSpriteNode["Offset"];
  if (tileOffsetNode.IsDefined() && tileOffsetNode.IsSequence()) {
    spriteDefinition.offset.x = tileOffsetNode[0].as<float>(0);
    spriteDefinition.offset.y = tileOffsetNode[1].as<float>(0);
  }

  auto tilingMode = isometricSpriteNode["TilingMode"];

  if (tilingMode.IsDefined()) {
    auto tilingModeString = tilingMode.as<std::string>();
    if (tilingModeString == "ISO_FLOOR") {
      spriteDefinition.tilingMode = TilingMode::ISO_FLOOR;
    }
  }

  isometricObserverDefinitions_.insert({renderTileName, spriteDefinition});
}

void GDYFactory::parseSpriteObserverDefinitions(std::string objectName, YAML::Node spriteNode) {
  if (!spriteNode.IsDefined()) {
    return;
  }

  if (spriteNode.IsSequence()) {
    for (std::size_t c = 0; c < spriteNode.size(); c++) {
      parseSpriteObserverDefinition(objectName, c, spriteNode[c]);
    }
  } else {
    parseSpriteObserverDefinition(objectName, 0, spriteNode);
  }
}

void GDYFactory::parseSpriteObserverDefinition(std::string objectName, uint32_t renderTileId, YAML::Node spriteNode) {
  SpriteDefinition spriteDefinition{};

  spriteDefinition.images = singleOrListNodeToList(spriteNode["Image"]);
  spriteDefinition.scale = spriteNode["Scale"].as<float>(1.0f);

  auto tilingMode = spriteNode["TilingMode"];

  if (tilingMode.IsDefined()) {
    auto tilingModeString = tilingMode.as<std::string>();
    if (tilingModeString == "WALL_2") {
      spriteDefinition.tilingMode = TilingMode::WALL_2;
    } else if (tilingModeString == "WALL_16") {
      spriteDefinition.tilingMode = TilingMode::WALL_16;
    }
  }

  std::string renderTileName = objectName + std::to_string(renderTileId);
  spdlog::debug("Adding sprite definition for {0}", renderTileName);
  spriteObserverDefinitions_.insert({renderTileName, spriteDefinition});
}

void GDYFactory::parseBlockObserverDefinitions(std::string objectName, YAML::Node blockNode) {
  if (!blockNode.IsDefined()) {
    return;
  }

  if (blockNode.IsSequence()) {
    for (std::size_t c = 0; c < blockNode.size(); c++) {
      parseBlockObserverDefinition(objectName, c, blockNode[c]);
    }
  } else {
    parseBlockObserverDefinition(objectName, 0, blockNode);
  }
}

void GDYFactory::parseBlockObserverDefinition(std::string objectName, uint32_t renderTileId, YAML::Node blockNode) {
  BlockDefinition blockDefinition;
  auto colorNode = blockNode["Color"];
  for (std::size_t c = 0; c < colorNode.size(); c++) {
    blockDefinition.color[c] = colorNode[c].as<float>();
  }
  blockDefinition.shape = blockNode["Shape"].as<std::string>();
  blockDefinition.scale = blockNode["Scale"].as<float>(1.0f);

  std::string renderTileName = objectName + std::to_string(renderTileId);
  blockObserverDefinitions_.insert({renderTileName, blockDefinition});
}

ActionBehaviourDefinition GDYFactory::makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                              std::string objectName,
                                                              std::string associatedObjectName,
                                                              std::string actionName,
                                                              std::string commandName,
                                                              BehaviourCommandArguments commandArguments,
                                                              CommandList actionPreconditions,
                                                              CommandList conditionalCommands) {
  ActionBehaviourDefinition behaviourDefinition;
  behaviourDefinition.actionName = actionName;
  behaviourDefinition.behaviourType = behaviourType;
  behaviourDefinition.commandName = commandName;
  behaviourDefinition.commandArguments = commandArguments;
  behaviourDefinition.conditionalCommands = conditionalCommands;

  switch (behaviourType) {
    case ActionBehaviourType::SOURCE:
      behaviourDefinition.sourceObjectName = objectName;
      behaviourDefinition.destinationObjectName = associatedObjectName;
      behaviourDefinition.actionPreconditions = actionPreconditions;
      break;
    case ActionBehaviourType::DESTINATION:
      behaviourDefinition.destinationObjectName = objectName;
      behaviourDefinition.sourceObjectName = associatedObjectName;
      break;
  }

  return behaviourDefinition;
}

void GDYFactory::parseActionBehaviours(ActionBehaviourType actionBehaviourType, std::string objectName, std::string actionName, std::vector<std::string> associatedObjectNames, YAML::Node commandsNode, YAML::Node preconditionsNode) {
  spdlog::debug("Parsing {0} commands for action {1}, object {2}", commandsNode.size(), actionName, objectName);

  // Get preconditions
  CommandList actionPreconditions;

  if (preconditionsNode.IsDefined()) {
    for (auto&& c : preconditionsNode) {
      auto preconditionsIt = validateCommandPairNode(c);
      auto preconditionCommandName = preconditionsIt->first.as<std::string>();
      auto preconditionCommandArgumentsNode = preconditionsIt->second;

      auto preconditionCommandArgumentMap = singleOrListNodeToCommandArguments(preconditionCommandArgumentsNode);

      actionPreconditions.emplace_back(preconditionCommandName, preconditionCommandArgumentMap);
    }
  }

  // if there are no commands, just add a default command to "do nothing"
  if (commandsNode.size() == 0) {
    for (auto associatedObjectName : associatedObjectNames) {
      auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, objectName, associatedObjectName, actionName, "nop", {}, actionPreconditions, {});
      objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
    }
    return;
  }

  for (auto&& c : commandsNode) {
    auto commandIt = validateCommandPairNode(c);
    // iterate through keys
    auto commandName = commandIt->first.as<std::string>();
    auto commandNode = commandIt->second;

    spdlog::debug("Parsing command {0} for action {1}, object {2}", commandName, actionName, objectName);

    parseCommandNode(commandName, commandNode, actionBehaviourType, objectName, actionName, associatedObjectNames, actionPreconditions);
  }
}

void GDYFactory::parseCommandNode(
    std::string commandName,
    YAML::Node commandNode,
    ActionBehaviourType actionBehaviourType,
    std::string objectName,
    std::string actionName,
    std::vector<std::string> associatedObjectNames,
    CommandList actionPreconditions) {
  if (commandNode.IsMap()) {
    // TODO: don't really like this check being done here. should be pushed into the object class really?
    if (commandName == "exec") {
      // We have an execute action that we need to parse slightly differently

      BehaviourCommandArguments commandArgumentMap;

      for (YAML::const_iterator execArgNode = commandNode.begin(); execArgNode != commandNode.end(); ++execArgNode) {
        auto execArgName = execArgNode->first.as<std::string>();
        auto execArgValue = execArgNode->second;

        commandArgumentMap[execArgName] = execArgValue;
      }

      for (auto associatedObjectName : associatedObjectNames) {
        auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, objectName, associatedObjectName, actionName, commandName, commandArgumentMap, actionPreconditions, {});

        objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
      }

    } else {
      // We have a conditional command
      auto conditionArguments = commandNode["Arguments"];
      auto conditionSubCommands = commandNode["Commands"];

      auto commandArgumentMap = singleOrListNodeToCommandArguments(conditionArguments);

      CommandList parsedSubCommands;
      for (auto&& conditionSubCommand : conditionSubCommands) {
        auto subCommandIt = validateCommandPairNode(conditionSubCommand);
        auto subCommandName = subCommandIt->first.as<std::string>();
        auto subCommandArguments = subCommandIt->second;

        auto subCommandArgumentMap = singleOrListNodeToCommandArguments(subCommandArguments);

        spdlog::debug("Parsing subcommand {0} conditions", subCommandName);

        parsedSubCommands.emplace_back(subCommandName, subCommandArgumentMap);
      }

      for (auto associatedObjectName : associatedObjectNames) {
        auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, objectName, associatedObjectName, actionName, commandName, commandArgumentMap, actionPreconditions, parsedSubCommands);

        objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
      }
    }

  } else if (commandNode.IsSequence() || commandNode.IsScalar()) {
    auto commandArgumentMap = singleOrListNodeToCommandArguments(commandNode);
    for (auto associatedObjectName : associatedObjectNames) {
      auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, objectName, associatedObjectName, actionName, commandName, commandArgumentMap, actionPreconditions, {});
      objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
    }
  } else {
    throw std::invalid_argument(fmt::format("Badly defined command {0}", commandName));
  }
}

bool GDYFactory::loadActionTriggerDefinition(std::unordered_set<std::string> sourceObjectNames, std::unordered_set<std::string> destinationObjectNames, std::string actionName, YAML::Node triggerNode) {
  if (!triggerNode.IsDefined()) {
    return false;
  }

  spdlog::debug("Loading action trigger for action {0}", actionName);

  ActionInputsDefinition inputDefinition;
  inputDefinition.relative = false;
  inputDefinition.internal = true;
  inputDefinition.mapToGrid = false;

  actionInputsDefinitions_[actionName] = inputDefinition;

  ActionTriggerDefinition actionTriggerDefinition;
  actionTriggerDefinition.sourceObjectNames = sourceObjectNames;
  actionTriggerDefinition.destinationObjectNames = destinationObjectNames;
  actionTriggerDefinition.range = triggerNode["Range"].as<uint32_t>(1.0);

  auto triggerTypeString = triggerNode["Type"].as<std::string>("RANGE_BOX_AREA");

  if (triggerTypeString == "NONE") {
    actionTriggerDefinition.triggerType = TriggerType::NONE;
  } else if (triggerTypeString == "RANGE_BOX_BOUNDARY") {
    actionTriggerDefinition.triggerType = TriggerType::RANGE_BOX_BOUNDARY;
  } else if (triggerTypeString == "RANGE_BOX_AREA") {
    actionTriggerDefinition.triggerType = TriggerType::RANGE_BOX_AREA;
  } else {
    throw std::invalid_argument(fmt::format("Invalid TriggerType {0} for action '{1}'", triggerTypeString, actionName));
  }

  actionTriggerDefinitions_[actionName] = actionTriggerDefinition;

  return true;
}

void GDYFactory::loadActionInputsDefinition(std::string actionName, YAML::Node InputMappingNode) {
  spdlog::debug("Loading action mapping for action {0}", actionName);

  // Internal actions can only be called by using "exec" within other actions
  bool internal = InputMappingNode["Internal"].as<bool>(false);
  bool relative = InputMappingNode["Relative"].as<bool>(false);
  bool mapToGrid = InputMappingNode["MapToGrid"].as<bool>(false);

  ActionInputsDefinition inputDefinition;
  inputDefinition.relative = relative;
  inputDefinition.internal = internal;

  inputDefinition.mapToGrid = mapToGrid;

  if (!internal) {
    externalActionNames_.push_back(actionName);
  }

  if (!mapToGrid) {
    auto inputMappingNode = InputMappingNode["Inputs"];
    if (!inputMappingNode.IsDefined()) {
      inputDefinition.inputMappings = defaultActionInputMappings();

    } else {
      for (YAML::const_iterator mappingNode = inputMappingNode.begin(); mappingNode != inputMappingNode.end(); ++mappingNode) {
        auto actionId = mappingNode->first.as<uint32_t>();

        InputMapping inputMapping;
        auto mappingNodeData = mappingNode->second;

        auto vectorToDestNode = mappingNodeData["VectorToDest"];
        if (vectorToDestNode.IsDefined()) {
          glm::ivec2 vector = {
              vectorToDestNode[0].as<int32_t>(0),
              vectorToDestNode[1].as<int32_t>(0)};

          inputMapping.vectorToDest = vector;
        }

        auto oreintationVectorNode = mappingNodeData["OrientationVector"];
        if (oreintationVectorNode.IsDefined()) {
          glm::ivec2 vector = {
              oreintationVectorNode[0].as<int32_t>(0),
              oreintationVectorNode[1].as<int32_t>(0)};

          inputMapping.orientationVector = vector;
        }

        auto descriptionNode = mappingNodeData["Description"];
        if (descriptionNode.IsDefined()) {
          inputMapping.description = descriptionNode.as<std::string>();
        }

        auto metaDataNode = mappingNodeData["MetaData"];
        if (metaDataNode.IsDefined()) {
          for (YAML::const_iterator it = metaDataNode.begin(); it != metaDataNode.end(); ++it) {
            auto key = it->first.as<std::string>();
            auto value = it->second.as<int32_t>();
            inputMapping.metaData[key] = value;
          }
        }

        inputDefinition.inputMappings[actionId] = inputMapping;
      }
    }
  }

  actionInputsDefinitions_[actionName] = inputDefinition;
}

void GDYFactory::loadActions(YAML::Node actions) {
  spdlog::info("Loading {0} actions...", actions.size());
  for (auto&& i : actions) {
    auto action = i;
    auto actionName = action["Name"].as<std::string>();
    auto probability = action["Probability"].as<float>(1.0);
    auto behavioursNode = action["Behaviours"];
    auto triggerNode = action["Trigger"];

    actionProbabilities_[actionName] = probability;

    std::unordered_set<std::string> allSrcObjectNames;
    std::unordered_set<std::string> allDstObjectNames;

    for (auto&& b : behavioursNode) {
      auto behaviourNode = b;
      auto srcNode = behaviourNode["Src"];
      auto dstNode = behaviourNode["Dst"];

      auto srcObjectNames = singleOrListNodeToList(srcNode["Object"]);
      auto dstObjectNames = singleOrListNodeToList(dstNode["Object"]);

      allSrcObjectNames.insert(srcObjectNames.begin(), srcObjectNames.end());
      allDstObjectNames.insert(dstObjectNames.begin(), dstObjectNames.end());

      // If the source of the destintation is not supplied then assume the source or dest is _empty and has no commands
      if (srcObjectNames.size() == 0) {
        srcObjectNames = {"_empty"};
      }

      if (dstObjectNames.size() == 0) {
        dstObjectNames = {"_empty"};
      }

      for (auto srcName : srcObjectNames) {
        parseActionBehaviours(ActionBehaviourType::SOURCE, srcName, actionName, dstObjectNames, srcNode["Commands"], srcNode["Preconditions"]);
      }

      for (auto dstName : dstObjectNames) {
        parseActionBehaviours(ActionBehaviourType::DESTINATION, dstName, actionName, srcObjectNames, dstNode["Commands"], EMPTY_NODE);
      }
    }

    // If we have a Trigger definition then we dont process ActionInputDefinitions
    if (!loadActionTriggerDefinition(allSrcObjectNames, allDstObjectNames, actionName, triggerNode)) {
      loadActionInputsDefinition(actionName, action["InputMapping"]);
    }
  }

  objectGenerator_->setActionProbabilities(actionProbabilities_);
  objectGenerator_->setActionTriggerDefinitions(actionTriggerDefinitions_);
  objectGenerator_->setActionInputDefinitions(actionInputsDefinitions_);
}

std::shared_ptr<TerminationHandler> GDYFactory::createTerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) const {
  return terminationGenerator_->newInstance(grid, players);
}

std::unordered_map<uint32_t, InputMapping> GDYFactory::defaultActionInputMappings() const {
  std::unordered_map<uint32_t, InputMapping> defaultInputMappings{
      {1, InputMapping{{-1, 0}, {-1, 0}, "Left"}},
      {2, InputMapping{{0, -1}, {0, -1}, "Up"}},
      {3, InputMapping{{1, 0}, {1, 0}, "Right"}},
      {4, InputMapping{{0, 1}, {0, 1}, "Down"}}};

  return defaultInputMappings;
}

std::shared_ptr<Observer> GDYFactory::createObserver(std::shared_ptr<Grid> grid, std::string observerName, uint32_t playerCount, uint32_t playerId) {
  if (observerTypes_.find(observerName) == observerTypes_.end()) {
    auto error = fmt::format("No observer registered with name {0}", observerName);
    spdlog::error(error);
    throw std::invalid_argument(error);
  }

  auto observerType = observerTypes_.at(observerName);
  auto isGlobalObserver = playerId == 0;

  switch (observerType) {
    case ObserverType::ISOMETRIC: {
      spdlog::debug("Creating ISOMETRIC observer");
      if (getIsometricSpriteObserverDefinitions().size() == 0) {
        throw std::invalid_argument("Environment does not suport Isometric rendering.");
      }

      auto observer = std::make_shared<IsometricSpriteObserver>(IsometricSpriteObserver(grid, getIsometricSpriteObserverDefinitions()));
      auto observerConfig = generateConfigForObserver<IsometricSpriteObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      observerConfig.resourceConfig = resourceConfig_;
      observer->init(observerConfig);
      return observer;
    } break;
    case ObserverType::SPRITE_2D: {
      spdlog::debug("Creating SPRITE observer");
      if (getSpriteObserverDefinitions().size() == 0) {
        throw std::invalid_argument("Environment does not suport Sprite2D rendering.");
      }

      auto observer = std::make_shared<SpriteObserver>(SpriteObserver(grid, getSpriteObserverDefinitions()));
      auto observerConfig = generateConfigForObserver<VulkanGridObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      observerConfig.resourceConfig = resourceConfig_;
      observer->init(observerConfig);
      return observer;
    } break;
    case ObserverType::BLOCK_2D: {
      spdlog::debug("Creating BLOCK observer");
      if (getBlockObserverDefinitions().size() == 0) {
        throw std::invalid_argument("Environment does not suport Block2D rendering.");
      }

      auto observer = std::make_shared<BlockObserver>(BlockObserver(grid, getBlockObserverDefinitions()));
      auto observerConfig = generateConfigForObserver<VulkanGridObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      observerConfig.resourceConfig = resourceConfig_;
      observer->init(observerConfig);
      return observer;
    } break;
    case ObserverType::VECTOR: {
      spdlog::debug("Creating VECTOR observer");
      auto observer = std::make_shared<VectorObserver>(VectorObserver(grid));
      auto observerConfig = generateConfigForObserver<VectorObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      if (observerConfig.includePlayerId) {
        spdlog::debug("Player ID included");
      }
      observer->init(observerConfig);
      return observer;
    } break;
    case ObserverType::ASCII: {
      spdlog::debug("Creating ASCII observer");
      auto observer = std::make_shared<ASCIIObserver>(ASCIIObserver(grid));
      auto observerConfig = generateConfigForObserver<ASCIIObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      observer->init(observerConfig);
      return observer;
    } break;
    case ObserverType::NONE: {
      spdlog::debug("Creating NONE observer");
      auto observer = std::make_shared<NoneObserver>(NoneObserver(grid));
      auto observerConfig = generateConfigForObserver<ObserverConfig>(observerName, isGlobalObserver);
      observer->init(observerConfig);
      return observer;
    } break;
    default:
      return nullptr;
  }
}

std::vector<std::string> GDYFactory::getExternalActionNames() const {
  return externalActionNames_;
}

std::unordered_map<std::string, ActionInputsDefinition> GDYFactory::getActionInputsDefinitions() const {
  return actionInputsDefinitions_;
}

std::unordered_map<std::string, ActionTriggerDefinition> GDYFactory::getActionTriggerDefinitions() const {
  return actionTriggerDefinitions_;
}

std::shared_ptr<TerminationGenerator> GDYFactory::getTerminationGenerator() const {
  return terminationGenerator_;
}

std::shared_ptr<LevelGenerator> GDYFactory::getLevelGenerator(uint32_t level) const {
  if (level >= mapLevelGenerators_.size()) {
    auto error = fmt::format("Level {0} does not exist. Please choose a level Id less than {1}", level, mapLevelGenerators_.size());
    spdlog::error(error);
    throw std::invalid_argument(error);
  }
  return mapLevelGenerators_[static_cast<uint32_t>(level)];
}

std::shared_ptr<LevelGenerator> GDYFactory::getLevelGenerator(std::string levelString) const {
  auto levelStringStream = std::stringstream(levelString);

  auto mapGenerator = std::make_shared<MapGenerator>(MapGenerator(playerCount_, objectGenerator_));
  mapGenerator->parseFromStream(levelStringStream);

  return mapGenerator;
}

std::shared_ptr<ObjectGenerator> GDYFactory::getObjectGenerator() const {
  return objectGenerator_;
}

std::unordered_map<std::string, SpriteDefinition> GDYFactory::getIsometricSpriteObserverDefinitions() const {
  return isometricObserverDefinitions_;
}

std::unordered_map<std::string, SpriteDefinition> GDYFactory::getSpriteObserverDefinitions() const {
  return spriteObserverDefinitions_;
}

std::unordered_map<std::string, BlockDefinition> GDYFactory::getBlockObserverDefinitions() const {
  return blockObserverDefinitions_;
}

ObserverType& GDYFactory::getNamedObserverType(std::string observerName) {
  return observerTypes_.at(observerName);
}

std::unordered_map<std::string, GlobalVariableDefinition> GDYFactory::getGlobalVariableDefinitions() const {
  return globalVariableDefinitions_;
}

const std::string& GDYFactory::getPlayerObserverName() const {
  return playerObserverName_;
}

DefaultObserverConfig GDYFactory::getDefaultObserverConfig() const {
  return defaultObserverConfig_;
}

std::string GDYFactory::getAvatarObject() const {
  return avatarObject_;
}

uint32_t GDYFactory::getLevelCount() const {
  return static_cast<uint32_t>(mapLevelGenerators_.size());
}

std::string GDYFactory::getName() const {
  return name_;
}

ActionInputsDefinition GDYFactory::findActionInputsDefinition(std::string actionName) const {
  auto mapping = actionInputsDefinitions_.find(actionName);
  if (mapping != actionInputsDefinitions_.end()) {
    return mapping->second;
  } else {
    auto error = fmt::format("Cannot find action input mapping for action={0}", actionName);
    throw std::runtime_error(error);
  }
}

uint32_t GDYFactory::getPlayerCount() const {
  return playerCount_;
}

}  // namespace griddly
