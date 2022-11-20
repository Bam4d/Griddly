#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <filesystem>
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

#ifndef WASM

GDYFactory::GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator, ResourceConfig defaultResourceConfig)
    : objectGenerator_(std::move(objectGenerator)),
      terminationGenerator_(std::move(terminationGenerator)),
      defaultResourceConfig_(std::move(defaultResourceConfig)) {
#else
GDYFactory::GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator)
    : objectGenerator_(std::move(objectGenerator)),
      terminationGenerator_(std::move(terminationGenerator)) {
#endif
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
}

void GDYFactory::initializeFromFile(std::string filename) {
  spdlog::debug("Loading GDY file: {0}", filename);
  std::ifstream gdyFile;
  gdyFile.open(filename);

  if (gdyFile.fail()) {
    auto error = fmt::format("Cannot find the file {0}", filename);
    throwParserError(error);
  }
  parseFromStream(gdyFile);
}

void GDYFactory::parseFromStream(std::istream& stream) {
  auto gdyConfig = YAML::Load(stream);

  auto versionNode = gdyConfig["Version"];
  auto version = versionNode.as<float>(0.1);
  spdlog::debug("Loading GDY file Version: {0}.", version);

  auto environment = gdyConfig["Environment"];
  auto objects = gdyConfig["Objects"];
  auto actions = gdyConfig["Actions"];

  loadObjects(objects);
  loadActions(actions);
  loadEnvironment(environment);
}

void GDYFactory::loadEnvironment(YAML::Node environment) {
  spdlog::debug("Loading Environment...");

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

  // For backward compatibility
  registerObserverConfigNode("Vector", observerConfigNode["Vector"], true);
  registerObserverConfigNode("ASCII", observerConfigNode["ASCII"], true);
  registerObserverConfigNode("Entity", observerConfigNode["Entity"], true);
#ifndef WASM
  registerObserverConfigNode("Sprite2D", observerConfigNode["Sprite2D"], true);
  registerObserverConfigNode("Block2D", observerConfigNode["Block2D"], true);
  registerObserverConfigNode("Isometric", observerConfigNode["Isometric"], true);
#endif
  observerTypes_.insert({"None", ObserverType::NONE});
  registerObserverConfigNode("ASCII", observerConfigNode["ASCII"], true);

  parseGlobalVariables(environment["Variables"]);
  parseTerminationConditions(environment["Termination"]);

  auto levels = environment["Levels"];
  for (std::size_t l = 0; l < levels.size(); l++) {
    auto levelStringStream = std::stringstream(levels[l].as<std::string>());

    auto mapGenerator = std::make_shared<MapGenerator>(MapGenerator(playerCount_, objectGenerator_));
    mapGenerator->parseFromStream(levelStringStream);
    mapLevelGenerators_.push_back(mapGenerator);
  }

  spdlog::debug("Loaded {0} levels", mapLevelGenerators_.size());
}

void GDYFactory::registerObserverConfigNode(std::string observerName, YAML::Node observerConfigNode, bool useObserverNameAsType) {
  std::string observerTypeString;
  if (!useObserverNameAsType) {
    if (!observerConfigNode["Type"].IsDefined()) {
      auto error = fmt::format("Observers must have a ObserverType defined.");
      throwParserError(error);
    }

    observerTypeString = observerConfigNode["Type"].as<std::string>();
  } else {
    observerTypeString = observerName;
  }

  spdlog::debug("Parsing named observer config with observer name: {0} and type: {1}", observerName, observerTypeString);

  if (observerTypeString == "VECTOR" || observerTypeString == "Vector") {
    observerTypes_.insert({observerName, ObserverType::VECTOR});
  } else if (observerTypeString == "ASCII") {
    observerTypes_.insert({observerName, ObserverType::ASCII});
  } else if (observerTypeString == "ENTITY" || observerTypeString == "Entity") {
    observerTypes_.insert({observerName, ObserverType::ENTITY});
#ifndef WASM
  } else if (observerTypeString == "SPRITE_2D" || observerTypeString == "Sprite2D") {
    observerTypes_.insert({observerName, ObserverType::SPRITE_2D});
  } else if (observerTypeString == "BLOCK_2D" || observerTypeString == "Block2D") {
    observerTypes_.insert({observerName, ObserverType::BLOCK_2D});
  } else if (observerTypeString == "ISOMETRIC" || observerTypeString == "Isometric") {
    observerTypes_.insert({observerName, ObserverType::ISOMETRIC});
#else
  } else if (observerTypeString == "SPRITE_2D" || observerTypeString == "Sprite2D") {
    observerTypes_.insert({observerName, ObserverType::NONE});
  } else if (observerTypeString == "BLOCK_2D" || observerTypeString == "Block2D") {
    observerTypes_.insert({observerName, ObserverType::NONE});
  } else if (observerTypeString == "ISOMETRIC" || observerTypeString == "Isometric") {
    observerTypes_.insert({observerName, ObserverType::NONE});
#endif
  } else {
    auto error = fmt::format("Unknown or undefined observer type: {0}", observerTypeString);
    throwParserError(error);
  }
  observerConfigNodes_.insert({observerName, observerConfigNode});
}

template <class ObserverConfigType>
ObserverConfigType GDYFactory::generateConfigForObserver(std::string observerName, bool isGlobalObserver) {
  std::shared_ptr<ObserverConfig> config;
  switch (observerTypes_.at(observerName)) {
    case ObserverType::VECTOR:
      config = std::make_shared<VectorObserverConfig>(parseNamedVectorObserverConfig(observerName, isGlobalObserver));
      break;
    case ObserverType::ASCII:
      config = std::make_shared<ASCIIObserverConfig>(parseNamedASCIIObserverConfig(observerName, isGlobalObserver));
      break;
    case ObserverType::ENTITY:
      config = std::make_shared<EntityObserverConfig>(parseNamedEntityObserverConfig(observerName, isGlobalObserver));
      break;
    case ObserverType::NONE:
      config = std::make_shared<ObserverConfig>();
      break;
#ifndef WASM
    case ObserverType::SPRITE_2D:
      config = std::make_shared<SpriteObserverConfig>(parseNamedSpriteObserverConfig(observerName, isGlobalObserver));
      break;
    case ObserverType::BLOCK_2D:
      config = std::make_shared<BlockObserverConfig>(parseNamedBlockObserverConfig(observerName, isGlobalObserver));
      break;
    case ObserverType::ISOMETRIC:
      config = std::make_shared<IsometricSpriteObserverConfig>(parseNamedIsometricObserverConfig(observerName, isGlobalObserver));
      break;
#endif

    default: {
      throwParserError(fmt::format("Unknown Observer Type: {0}", observerName));
    }
  }

  return *std::static_pointer_cast<ObserverConfigType>(config);
}

template <class NodeValueType>
NodeValueType GDYFactory::resolveObserverConfigValue(std::string key, YAML::Node observerConfigNode, NodeValueType defaultValue, bool fallbackToDefaultConfig) {
  return observerConfigNode[key].as<NodeValueType>(fallbackToDefaultConfig ? defaultObserverConfigNode_[key].as<NodeValueType>(defaultValue) : defaultValue);
}

VectorObserverConfig GDYFactory::parseNamedVectorObserverConfig(std::string observerName, bool isGlobalObserver) {
  VectorObserverConfig config{};

  spdlog::debug("Parsing VECTOR observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);

  config.includePlayerId = resolveObserverConfigValue<bool>("IncludePlayerId", observerConfigNode, config.includePlayerId, !isGlobalObserver);
  config.includeRotation = resolveObserverConfigValue<bool>("IncludeRotation", observerConfigNode, config.includeRotation, !isGlobalObserver);
  config.includeVariables = resolveObserverConfigValue<bool>("IncludeVariables", observerConfigNode, config.includeVariables, !isGlobalObserver);

  auto globalVariableMappingNode = observerConfigNode["GlobalVariableMapping"];

  if (globalVariableMappingNode.IsDefined()) {
    const auto& globalEntityVariables = singleOrListNodeToList(globalVariableMappingNode);

    for (const auto& globalEntityVariable : globalEntityVariables) {
      if (globalVariableDefinitions_.find(globalEntityVariable) == globalVariableDefinitions_.end()) {
        std::string error = fmt::format("No global variable with name {0} in GlobalVariableMapping feature configuration.", globalEntityVariable);
        throwParserError(error);
      }
    }

    config.globalVariableMapping = globalEntityVariables;
  }

  return config;
}

ASCIIObserverConfig GDYFactory::parseNamedASCIIObserverConfig(std::string observerName, bool isGlobalObserver) {
  ASCIIObserverConfig config{};

  spdlog::debug("Parsing ASCII observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);

  config.asciiPadWidth = resolveObserverConfigValue<int32_t>("Padding", observerConfigNode, config.asciiPadWidth, !isGlobalObserver);
  config.includePlayerId = resolveObserverConfigValue<bool>("IncludePlayerId", observerConfigNode, config.includePlayerId, !isGlobalObserver);

  return config;
}

EntityObserverConfig GDYFactory::parseNamedEntityObserverConfig(std::string observerName, bool isGlobalObserver) {
  EntityObserverConfig config{};

  spdlog::debug("Parsing ENTITY observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);

  // Used to generate masks for entity obervers
  config.actionInputsDefinitions = getActionInputsDefinitions();

  auto globalVariableMappingNode = observerConfigNode["GlobalVariableMapping"];

  if (globalVariableMappingNode.IsDefined()) {
    const auto& globalEntityVariables = singleOrListNodeToList(globalVariableMappingNode);

    for (const auto& globalEntityVariable : globalEntityVariables) {
      if (globalVariableDefinitions_.find(globalEntityVariable) == globalVariableDefinitions_.end()) {
        std::string error = fmt::format("No global variable with name {0} in GlobalVariableMapping feature configuration.", globalEntityVariable);
        throwParserError(error);
      }
    }

    config.globalVariableMapping = globalEntityVariables;
  }

  auto variableMappingNodes = observerConfigNode["VariableMapping"];

  if (variableMappingNodes.IsDefined()) {
    for (YAML::const_iterator variableMappingNode = variableMappingNodes.begin(); variableMappingNode != variableMappingNodes.end(); ++variableMappingNode) {
      const auto& entityName = variableMappingNode->first.as<std::string>();

      if (objectNames_.find(entityName) == objectNames_.end()) {
        std::string error = fmt::format("No entity with name {0} in entity observer variable mapping configuration.", entityName);
        throwParserError(error);
      }

      const auto& entityVariableMapping = variableMappingNode->second;

      const auto& entityVariables = singleOrListNodeToList(entityVariableMapping);

      config.entityVariableMapping[entityName] = entityVariables;
    }
  }

  auto includePlayerIdEntities = singleOrListNodeToList(observerConfigNode["IncludePlayerId"]);
  auto includeRotationEntities = singleOrListNodeToList(observerConfigNode["IncludeRotation"]);
  config.includePlayerId = std::unordered_set<std::string>(includePlayerIdEntities.begin(), includePlayerIdEntities.end());
  config.includeRotation = std::unordered_set<std::string>(includeRotationEntities.begin(), includeRotationEntities.end());

  config.includeMasks = resolveObserverConfigValue<bool>("IncludeMasks", observerConfigNode, config.includeMasks, !isGlobalObserver);

  for (const auto& playerIdEntityName : config.includePlayerId) {
    if (objectNames_.find(playerIdEntityName) == objectNames_.end()) {
      std::string error = fmt::format("No entity with name {0} in entity observer playerId feature configuration.", playerIdEntityName);
      throwParserError(error);
    }
  }

  for (const auto& rotationEntityName : config.includeRotation) {
    if (objectNames_.find(rotationEntityName) == objectNames_.end()) {
      std::string error = fmt::format("No entity with name {0} in entity observer rotation feature configuration.", rotationEntityName);
      throwParserError(error);
    }
  }

  config.objectNames.insert(config.objectNames.end(), objectNames_.begin(), objectNames_.end());

  return config;
}

#ifndef WASM
SpriteObserverConfig GDYFactory::parseNamedSpriteObserverConfig(std::string observerName, bool isGlobalObserver) {
  SpriteObserverConfig config{};

  spdlog::debug("Parsing SPRITE observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);
  parseNamedObserverShaderConfig(config, observerConfigNode);

  parseNamedObserverResourceConfig(config, observerConfigNode, "/default/sprite");

  config.playerColors = playerColors_;
  config.tileSize = parseTileSize(observerConfigNode);
  config.highlightPlayers = resolveObserverConfigValue<bool>("HighlightPlayers", observerConfigNode, playerCount_ > 1, !isGlobalObserver);
  config.rotateAvatarImage = resolveObserverConfigValue<bool>("RotateAvatarImage", observerConfigNode, config.rotateAvatarImage, !isGlobalObserver);

  auto backgroundTileNode = observerConfigNode["BackgroundTile"];

  if (backgroundTileNode.IsDefined()) {
    auto backgroundTile = backgroundTileNode.as<std::string>();
    spdlog::debug("Setting background tiling to {0}", backgroundTile);
    SpriteDefinition backgroundTileDefinition{};
    backgroundTileDefinition.images = {backgroundTile};
    config.spriteDefinitions.insert({"_background_", backgroundTileDefinition});
  }

  auto paddingTileNode = observerConfigNode["PaddingTile"];
  if (paddingTileNode.IsDefined()) {
    auto paddingTile = paddingTileNode.as<std::string>();
    spdlog::debug("Setting padding tiling to {0}", paddingTile);
    SpriteDefinition paddingTileDefiniton{};
    paddingTileDefiniton.images = {paddingTile};
    config.spriteDefinitions.insert({"_padding_", paddingTileDefiniton});
  }

  if (objectNames_.size() == 0) {
    return config;
  }

  if (objectObserverConfigNodes_.find(observerName) != objectObserverConfigNodes_.end()) {
    const auto& objectObserverConfigNode = objectObserverConfigNodes_.at(observerName);
    if (objectNames_.size() != objectObserverConfigNode.size()) {
      auto error = fmt::format("Objects are missing configuration keys for observer: {0}", observerName);
      throwParserError(error);
    }
    parseObjectSpriteObserverDefinitions(config, objectObserverConfigNode);
  } else {
    auto error = fmt::format("Objects are missing configuration keys for observer: {0}", observerName);
    throwParserError(error);
  }

  return config;
}

BlockObserverConfig GDYFactory::parseNamedBlockObserverConfig(std::string observerName, bool isGlobalObserver) {
  BlockObserverConfig config{};

  spdlog::debug("Parsing BLOCK observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);
  parseNamedObserverShaderConfig(config, observerConfigNode);

  parseNamedObserverResourceConfig(config, observerConfigNode, "/default/block");

  config.spriteDefinitions = BlockObserver::blockSpriteDefinitions_;
  config.playerColors = playerColors_;
  config.tileSize = parseTileSize(observerConfigNode);
  config.highlightPlayers = resolveObserverConfigValue<bool>("HighlightPlayers", observerConfigNode, playerCount_ > 1, !isGlobalObserver);
  config.rotateAvatarImage = resolveObserverConfigValue<bool>("RotateAvatarImage", observerConfigNode, config.rotateAvatarImage, !isGlobalObserver);

  if (objectNames_.size() == 0) {
    return config;
  }

  if (objectObserverConfigNodes_.find(observerName) != objectObserverConfigNodes_.end()) {
    const auto& objectObserverConfigNode = objectObserverConfigNodes_.at(observerName);
    if (objectNames_.size() != objectObserverConfigNode.size()) {
      auto error = fmt::format("Objects are missing configuration keys for observer: {0}", observerName);
      throwParserError(error);
    }
    parseObjectBlockObserverDefinitions(config, objectObserverConfigNode);
  } else {
    auto error = fmt::format("Objects are missing configuration keys for observer: {0}", observerName);
    throwParserError(error);
  }

  return config;
}

IsometricSpriteObserverConfig GDYFactory::parseNamedIsometricObserverConfig(std::string observerName, bool isGlobalObserver) {
  IsometricSpriteObserverConfig config{};

  spdlog::debug("Parsing ISOMETRIC observer config with observer name: {0}", observerName);

  auto observerConfigNode = observerConfigNodes_.at(observerName);
  parseCommonObserverConfig(config, observerConfigNode, isGlobalObserver);
  parseNamedObserverShaderConfig(config, observerConfigNode);

  parseNamedObserverResourceConfig(config, observerConfigNode, "/default/isometric");

  config.playerColors = playerColors_;
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
    config.spriteDefinitions.insert({"_iso_background_", backgroundTileDefinition});
  }

  if (objectNames_.size() == 0) {
    return config;
  }

  if (objectObserverConfigNodes_.find(observerName) != objectObserverConfigNodes_.end()) {
    const auto& objectObserverConfigNodes = objectObserverConfigNodes_.at(observerName);
    if (objectNames_.size() != objectObserverConfigNodes.size()) {
      auto error = fmt::format("Objects are missing configuration keys for observer: {0}", observerName);
      throwParserError(error);
    }
    parseObjectIsometricObserverDefinitions(config, objectObserverConfigNodes);
  } else {
    auto error = fmt::format("Objects are missing configuration keys for observer: {0}", observerName);
    throwParserError(error);
  }

  return config;
}

void GDYFactory::parseNamedObserverResourceConfig(VulkanObserverConfig& config, YAML::Node observerConfigNode, std::string defaultShaderPath) {
  auto resourceConfigNode = observerConfigNode["ResourceConfig"];
  config.resourceConfig = defaultResourceConfig_;
  config.resourceConfig.shaderPath = (std::filesystem::path(config.resourceConfig.shaderPath) += std::filesystem::path(defaultShaderPath)).string();

  if (!resourceConfigNode.IsDefined()) {
    spdlog::debug("Using default Resource Config");
    return;
  }

  if (resourceConfigNode["ImagePath"].IsDefined()) {
    config.resourceConfig.imagePath = std::filesystem::path(resourceConfigNode["ImagePath"].as<std::string>()).string();
  }

  if (resourceConfigNode["ShaderPath"].IsDefined()) {
    config.resourceConfig.shaderPath = std::filesystem::path(resourceConfigNode["ShaderPath"].as<std::string>()).string();
  }
};

void GDYFactory::parseNamedObserverShaderConfig(VulkanObserverConfig& config, YAML::Node observerConfigNode) {
  auto shaderConfigNode = observerConfigNode["Shader"];
  if (!shaderConfigNode.IsDefined()) {
    spdlog::debug("Passing no additional variables to shaders");
    return;
  }

  auto globalObserverAvatarMode = shaderConfigNode["ObserverAvatarMode"];
  if (globalObserverAvatarMode.IsDefined()) {
    auto avatarMode = globalObserverAvatarMode.as<std::string>();
    if (avatarMode == "GRAYSCALE") {
      config.globalObserverAvatarMode = GlobalObserverAvatarMode::GRAYSCALE_INVISIBLE;
    } else if (avatarMode == "DARKEN") {
      config.globalObserverAvatarMode = GlobalObserverAvatarMode::DARKEN_INVISIBLE;
    } else if (avatarMode == "REMOVE") {
      config.globalObserverAvatarMode = GlobalObserverAvatarMode::REMOVE_INVISIBLE;
    } else {
      std::string error = fmt::format("No avatar mode called {0} exists.", avatarMode);
      throwParserError(error);
    }
  }

  auto globalVariableNode = shaderConfigNode["GlobalVariables"];
  if (globalVariableNode.IsDefined()) {
    for (std::size_t i = 0; i < globalVariableNode.size(); i++) {
      auto globalVariableName = globalVariableNode[i].as<std::string>();

      // Check the variable exists
      if (globalVariableDefinitions_.find(globalVariableName) == globalVariableDefinitions_.end()) {
        std::string error = fmt::format("No global variable with name {0} exists to expose to shaders", globalVariableName);
        throwParserError(error);
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
        throwParserError(error);
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

#endif

void GDYFactory::parseCommonObserverConfig(ObserverConfig& observerConfig, YAML::Node observerConfigNode, bool isGlobalObserver) {
  spdlog::debug("Parsing common observer config...");
  observerConfig.overrideGridWidth = resolveObserverConfigValue<int32_t>("Width", observerConfigNode, observerConfig.overrideGridWidth, !isGlobalObserver);
  observerConfig.overrideGridHeight = resolveObserverConfigValue<int32_t>("Height", observerConfigNode, observerConfig.overrideGridHeight, !isGlobalObserver);
  observerConfig.gridXOffset = resolveObserverConfigValue<int32_t>("OffsetX", observerConfigNode, observerConfig.gridXOffset, !isGlobalObserver);
  observerConfig.gridYOffset = resolveObserverConfigValue<int32_t>("OffsetY", observerConfigNode, observerConfig.gridYOffset, !isGlobalObserver);
  observerConfig.trackAvatar = resolveObserverConfigValue<bool>("TrackAvatar", observerConfigNode, observerConfig.trackAvatar, !isGlobalObserver);
  observerConfig.rotateWithAvatar = resolveObserverConfigValue<bool>("RotateWithAvatar", observerConfigNode, observerConfig.rotateWithAvatar, !isGlobalObserver);
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

  auto playerColorNode = playerNode["Colors"];
  if (playerColorNode.IsDefined()) {
    for (auto&& p : playerColorNode) {
      if (!p.IsSequence() || p.size() != 3) {
        auto error = fmt::format("Player color node misconfigured, must contain 3 values but only contains.", p.size());
        throwParserError(error);
      } else {
        glm::vec3 color(
            p[0].as<float>(0),
            p[1].as<float>(0),
            p[2].as<float>(0));
        playerColors_.push_back(color);
      }
    }
  }
}

void GDYFactory::parseTerminationConditionV1(TerminationState state, YAML::Node conditionNode) {
  terminationGenerator_->defineTerminationCondition(state, 0, 0, conditionNode);
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

    terminationGenerator_->defineTerminationCondition(state, reward, opposingReward, conditionNode);
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
  auto maxStepsGDY = fmt::format("gt: [_steps, {0}]", std::to_string(maxSteps));
  auto maxStepsNode = YAML::Load(maxStepsGDY);
  terminationGenerator_->defineTerminationCondition(TerminationState::LOSE, 0, 0, maxStepsNode);
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
  spdlog::debug("Loading {0} objects...", objects.size());

  for (auto&& i : objects) {
    auto object = i;
    auto objectName = object["Name"].as<std::string>();
    auto mapCharacter = object["MapCharacter"].as<char>('?');
    auto observerDefinitions = object["Observers"];

    objectNames_.insert(objectName);

#ifndef WASM
    if (observerDefinitions.IsDefined()) {
      for (YAML::const_iterator observerDefinitionNode = observerDefinitions.begin(); observerDefinitionNode != observerDefinitions.end(); ++observerDefinitionNode) {
        const auto& observerName = observerDefinitionNode->first.as<std::string>();
        spdlog::debug("Adding object observer config node for observer {0}, object: {1}", observerName, objectName);
        objectObserverConfigNodes_[observerName][objectName] = observerDefinitions[observerName];
      }
    }
#endif

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
}

#ifndef WASM
void GDYFactory::parseObjectIsometricObserverDefinitions(IsometricSpriteObserverConfig& observerConfig, std::unordered_map<std::string, YAML::Node> objectObserverConfigNodes) {
  for (const auto& objectObserverConfigNode : objectObserverConfigNodes) {
    const auto& objectName = objectObserverConfigNode.first;
    const auto& observerNode = objectObserverConfigNode.second;

    if (observerNode.IsSequence()) {
      for (std::size_t c = 0; c < observerNode.size(); c++) {
        parseObjectIsometricObserverDefinition(observerConfig, objectName, c, observerNode[c]);
      }
    } else {
      parseObjectIsometricObserverDefinition(observerConfig, objectName, 0, observerNode);
    }
  }
}

void GDYFactory::parseObjectIsometricObserverDefinition(IsometricSpriteObserverConfig& observerConfig, std::string objectName, uint32_t renderTileId, YAML::Node isometricSpriteNode) {
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

  observerConfig.spriteDefinitions.insert({renderTileName, spriteDefinition});
}

void GDYFactory::parseObjectSpriteObserverDefinitions(SpriteObserverConfig& observerConfig, std::unordered_map<std::string, YAML::Node> objectObserverConfigNodes) {
  for (const auto& objectObserverConfigNode : objectObserverConfigNodes) {
    const auto& objectName = objectObserverConfigNode.first;
    const auto& observerNode = objectObserverConfigNode.second;

    if (observerNode.IsSequence()) {
      for (std::size_t c = 0; c < observerNode.size(); c++) {
        parseObjectSpriteObserverDefinition(observerConfig, objectName, c, observerNode[c]);
      }
    } else {
      parseObjectSpriteObserverDefinition(observerConfig, objectName, 0, observerNode);
    }
  }
}

void GDYFactory::parseObjectSpriteObserverDefinition(SpriteObserverConfig& observerConfig, std::string objectName, uint32_t renderTileId, YAML::Node spriteNode) {
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
  observerConfig.spriteDefinitions.insert({renderTileName, spriteDefinition});
}

void GDYFactory::parseObjectBlockObserverDefinitions(BlockObserverConfig& observerConfig, std::unordered_map<std::string, YAML::Node> objectObserverConfigNodes) {
  for (const auto& objectObserverConfigNode : objectObserverConfigNodes) {
    const auto& objectName = objectObserverConfigNode.first;
    const auto& observerNode = objectObserverConfigNode.second;
    if (observerNode.IsSequence()) {
      for (std::size_t c = 0; c < observerNode.size(); c++) {
        parseObjectBlockObserverDefinition(observerConfig, objectName, c, observerNode[c]);
      }
    } else {
      parseObjectBlockObserverDefinition(observerConfig, objectName, 0, observerNode);
    }
  }
}

void GDYFactory::parseObjectBlockObserverDefinition(BlockObserverConfig& observerConfig, std::string objectName, uint32_t renderTileId, YAML::Node blockNode) {
  BlockDefinition blockDefinition;
  auto colorNode = blockNode["Color"];

  if (colorNode.IsDefined()) {
    if (colorNode.IsSequence()) {
      for (std::size_t c = 0; c < colorNode.size(); c++) {
        blockDefinition.color[c] = colorNode[c].as<float>();
      }
    } else {
      auto colorString = colorNode.as<std::string>();
      if (colorString == "PLAYER") {
        blockDefinition.usePlayerColor = true;
      }
    }
  }
  blockDefinition.shape = blockNode["Shape"].as<std::string>();
  blockDefinition.scale = blockNode["Scale"].as<float>(1.0f);

  std::string renderTileName = objectName + std::to_string(renderTileId);
  observerConfig.blockDefinitions.insert({renderTileName, blockDefinition});
}

#endif

ActionBehaviourDefinition GDYFactory::makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                              uint32_t behaviourIdx,
                                                              std::string objectName,
                                                              std::string associatedObjectName,
                                                              std::string actionName,
                                                              std::string commandName,
                                                              CommandArguments commandArguments,
                                                              YAML::Node actionPreconditionsNode,
                                                              CommandList conditionalCommands) {
  ActionBehaviourDefinition behaviourDefinition;
  behaviourDefinition.behaviourIdx = behaviourIdx;
  behaviourDefinition.actionName = actionName;
  behaviourDefinition.behaviourType = behaviourType;
  behaviourDefinition.commandName = commandName;
  behaviourDefinition.commandArguments = commandArguments;
  behaviourDefinition.conditionalCommands = conditionalCommands;

  switch (behaviourType) {
    case ActionBehaviourType::SOURCE:
      behaviourDefinition.sourceObjectName = objectName;
      behaviourDefinition.destinationObjectName = associatedObjectName;
      behaviourDefinition.actionPreconditionsNode = actionPreconditionsNode;
      break;
    case ActionBehaviourType::DESTINATION:
      behaviourDefinition.destinationObjectName = objectName;
      behaviourDefinition.sourceObjectName = associatedObjectName;
      break;
  }

  return behaviourDefinition;
}

void GDYFactory::parseActionBehaviours(ActionBehaviourType actionBehaviourType, uint32_t behaviourIdx, std::string objectName, std::string actionName, std::vector<std::string> associatedObjectNames, YAML::Node commandsNode, YAML::Node preconditionsNode) {
  spdlog::debug("Parsing {0} commands for action {1}, object {2}", commandsNode.size(), actionName, objectName);

  if (objectName != "_empty" && objectName != "_boundary" && objectNames_.find(objectName) == objectNames_.end()) {
    auto error = fmt::format("Object with name {0} does not exist", objectName);
    throwParserError(error);
  }

  // if there are no commands, just add a default command to "do nothing"
  if (commandsNode.size() == 0) {
    for (auto associatedObjectName : associatedObjectNames) {
      auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, behaviourIdx, objectName, associatedObjectName, actionName, "nop", {}, preconditionsNode, {});
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

    parseCommandNode(commandName, commandNode, actionBehaviourType, behaviourIdx, objectName, actionName, associatedObjectNames, preconditionsNode);
  }
}

void GDYFactory::parseCommandNode(
    std::string commandName,
    YAML::Node commandNode,
    ActionBehaviourType actionBehaviourType,
    uint32_t behaviourIdx,
    std::string objectName,
    std::string actionName,
    std::vector<std::string> associatedObjectNames,
    YAML::Node preconditionsNode) {
  if (commandNode.IsMap()) {
    // TODO: don't really like this check being done here. should be pushed into the object class really?
    if (commandName == "exec" || commandName == "if") {
      // We have an execute action that we need to parse slightly differently

      CommandArguments commandArgumentMap;

      for (YAML::const_iterator execArgNode = commandNode.begin(); execArgNode != commandNode.end(); ++execArgNode) {
        auto execArgName = execArgNode->first.as<std::string>();
        auto execArgValue = execArgNode->second;

        commandArgumentMap[execArgName] = execArgValue;
      }

      for (auto associatedObjectName : associatedObjectNames) {
        auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, behaviourIdx, objectName, associatedObjectName, actionName, commandName, commandArgumentMap, preconditionsNode, {});

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
        auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, behaviourIdx, objectName, associatedObjectName, actionName, commandName, commandArgumentMap, preconditionsNode, parsedSubCommands);

        objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
      }
    }

  } else if (commandNode.IsSequence() || commandNode.IsScalar()) {
    auto commandArgumentMap = singleOrListNodeToCommandArguments(commandNode);
    for (auto associatedObjectName : associatedObjectNames) {
      auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, behaviourIdx, objectName, associatedObjectName, actionName, commandName, commandArgumentMap, preconditionsNode, {});
      objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
    }
  } else {
    throwParserError(fmt::format("Badly defined command {0}", commandName));
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
    throwParserError(fmt::format("Invalid TriggerType {0} for action '{1}'", triggerTypeString, actionName));
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
  spdlog::debug("Loading {0} actions...", actions.size());
  for (auto&& i : actions) {
    auto action = i;
    auto actionName = action["Name"].as<std::string>();
    auto probability = action["Probability"].as<float>(1.0);
    auto behavioursNode = action["Behaviours"];
    auto triggerNode = action["Trigger"];

    std::unordered_set<std::string> allSrcObjectNames;
    std::unordered_set<std::string> allDstObjectNames;

    for (std::size_t i = 0; i < behavioursNode.size(); i++) {
      auto&& behaviourNode = behavioursNode[i];
      auto srcNode = behaviourNode["Src"];
      auto dstNode = behaviourNode["Dst"];
      auto behaviourProb = behaviourNode["Probability"].as<float>(probability);

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

      behaviourProbabilities_[actionName].push_back(behaviourProb);

      for (auto srcName : srcObjectNames) {
        parseActionBehaviours(ActionBehaviourType::SOURCE, i, srcName, actionName, dstObjectNames, srcNode["Commands"], srcNode["Preconditions"]);
      }

      for (auto dstName : dstObjectNames) {
        parseActionBehaviours(ActionBehaviourType::DESTINATION, i, dstName, actionName, srcObjectNames, dstNode["Commands"], EMPTY_NODE);
      }
    }

    // If we have a Trigger definition then we dont process ActionInputDefinitions
    if (!loadActionTriggerDefinition(allSrcObjectNames, allDstObjectNames, actionName, triggerNode)) {
      loadActionInputsDefinition(actionName, action["InputMapping"]);
    }
  }

  objectGenerator_->setBehaviourProbabilities(behaviourProbabilities_);
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
    throwParserError(error);
  }

  auto observerType = observerTypes_.at(observerName);
  auto isGlobalObserver = playerId == 0;

  std::vector<std::shared_ptr<Observer>> observers;

  switch (observerType) {
#ifndef WASM
    case ObserverType::ISOMETRIC: {
      spdlog::debug("Creating ISOMETRIC observer from config: {0}", observerName);
      auto observerConfig = generateConfigForObserver<IsometricSpriteObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      return std::make_shared<IsometricSpriteObserver>(IsometricSpriteObserver(grid, observerConfig));
    } break;
    case ObserverType::SPRITE_2D: {
      spdlog::debug("Creating SPRITE observer from config: {0}", observerName);

      auto observerConfig = generateConfigForObserver<SpriteObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      return std::make_shared<SpriteObserver>(SpriteObserver(grid, observerConfig));
    } break;
    case ObserverType::BLOCK_2D: {
      spdlog::debug("Creating BLOCK observer from config: {0}", observerName);

      auto observerConfig = generateConfigForObserver<BlockObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      return std::make_shared<BlockObserver>(BlockObserver(grid, observerConfig));
    } break;
#endif
    case ObserverType::VECTOR: {
      spdlog::debug("Creating VECTOR observer from config: {0}", observerName);

      auto observerConfig = generateConfigForObserver<VectorObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;
      return std::make_shared<VectorObserver>(VectorObserver(grid, observerConfig));
    } break;
    case ObserverType::ASCII: {
      spdlog::debug("Creating ASCII observer from config: {0}", observerName);

      auto observerConfig = generateConfigForObserver<ASCIIObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;

      return std::make_shared<ASCIIObserver>(ASCIIObserver(grid, observerConfig));
    } break;
    case ObserverType::ENTITY: {
      spdlog::debug("Creating ENTITY observer from config: {0}", observerName);

      auto observerConfig = generateConfigForObserver<EntityObserverConfig>(observerName, isGlobalObserver);
      observerConfig.playerCount = playerCount;
      observerConfig.playerId = playerId;

      return std::make_shared<EntityObserver>(EntityObserver(grid, observerConfig));
      ;
    } break;
    case ObserverType::NONE: {
      spdlog::debug("Creating NONE observer from config: {0}", observerName);

      auto observerConfig = generateConfigForObserver<ObserverConfig>(observerName, isGlobalObserver);

      return std::make_shared<NoneObserver>(NoneObserver(grid, observerConfig));
      ;
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
    throwParserError(error);
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
