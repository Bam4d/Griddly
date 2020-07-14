#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "../Grid.hpp"
#include "../LevelGenerators/MapReader.hpp"
#include "../Observers/VectorObserver.hpp"
#include "../TurnBasedGameProcess.hpp"
#include "GDYFactory.hpp"

#define EMPTY_NODE YAML::Node()

namespace griddly {

GDYFactory::GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator)
    : objectGenerator_(objectGenerator),
      terminationGenerator_(terminationGenerator) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
}

GDYFactory::~GDYFactory() {
}

void GDYFactory::createLevel(uint32_t width, uint32_t height, std::shared_ptr<Grid> grid) {
  grid->resetMap(width, height);
}

void GDYFactory::loadLevel(uint32_t level) {
  if (mapReaderLevelGenerator_ == nullptr) {
    mapReaderLevelGenerator_ = std::shared_ptr<MapReader>(new MapReader(objectGenerator_));
  }

  auto levelStringStream = std::stringstream(levelStrings_[level]);
  mapReaderLevelGenerator_->parseFromStream(levelStringStream);
}

void GDYFactory::loadLevelString(std::string levelString) {
  if (mapReaderLevelGenerator_ == nullptr) {
    mapReaderLevelGenerator_ = std::shared_ptr<MapReader>(new MapReader(objectGenerator_));
  }

  auto levelStringStream = std::stringstream(levelString);
  mapReaderLevelGenerator_->parseFromStream(levelStringStream);
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
  if (versionNode.IsDefined()) {
    auto version = versionNode.as<std::string>();
    spdlog::info("Loading GDY file Version: {0}.", version);
  } else {
    spdlog::warn("No GDY version specified. Defaulting to Version: 0.1.");
  }

  auto environment = gdyConfig["Environment"];
  auto objects = gdyConfig["Objects"];
  auto actions = gdyConfig["Actions"];

  loadObjects(objects);
  loadActions(actions);

  loadEnvironment(environment);
}

void GDYFactory::loadEnvironment(YAML::Node environment) {
  spdlog::info("Loading Environment...");

  if (environment["TileSize"].IsDefined()) {
    tileSize_ = environment["TileSize"].as<uint32_t>();
    spdlog::debug("Setting tile size: {0}", tileSize_);
  }

  if (environment["Name"].IsDefined()) {
    name_ = environment["Name"].as<std::string>();
    spdlog::debug("Setting environment name: {0}", name_);
  }

  auto backgroundTileNode = environment["BackgroundTile"];
  if (backgroundTileNode.IsDefined()) {
    auto backgroundTile = backgroundTileNode.as<std::string>();
    spdlog::debug("Setting background tiling to {0}", backgroundTile);
    SpriteDefinition backgroundTileDefinition;
    backgroundTileDefinition.images = {backgroundTile};
    spriteObserverDefinitions_.insert({"_background_", backgroundTileDefinition});
  }

  auto levels = environment["Levels"];
  for (std::size_t l = 0; l < levels.size(); l++) {
    auto levelString = levels[l].as<std::string>();
    levelStrings_.push_back(levelString);
  }

  parsePlayerDefinition(environment["Player"]);
  parseGlobalVariables(environment["Variables"]);
  parseTerminationConditions(environment["Termination"]);

  spdlog::info("Loaded {0} levels", levelStrings_.size());
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
      auto observerGridWidth = observerNode["Width"].as<uint32_t>(0);
      auto observerGridHeight = observerNode["Height"].as<uint32_t>(0);
      auto observerGridOffsetX = observerNode["OffsetX"].as<uint32_t>(0);
      auto observerGridOffsetY = observerNode["OffsetY"].as<uint32_t>(0);
      auto trackAvatar = observerNode["TrackAvatar"].as<bool>(false);
      auto rotateWithAvatar = observerNode["RotateWithAvatar"].as<bool>(false);

      playerObserverDefinition_.gridHeight = observerGridHeight;
      playerObserverDefinition_.gridWidth = observerGridWidth;
      playerObserverDefinition_.gridXOffset = observerGridOffsetX;
      playerObserverDefinition_.gridYOffset = observerGridOffsetY;
      playerObserverDefinition_.trackAvatar = trackAvatar;
      playerObserverDefinition_.rotateWithAvatar = rotateWithAvatar;
    }
  }
}

void GDYFactory::parseTerminationConditions(YAML::Node terminationNode) {
  if (!terminationNode.IsDefined()) {
    return;
  }

  auto winNode = terminationNode["Win"];
  if (winNode.IsDefined()) {
    spdlog::debug("Parsing win conditions.");
    for (std::size_t c = 0; c < winNode.size(); c++) {
      auto commandIt = winNode[c].begin();
      auto commandName = commandIt->first.as<std::string>();
      auto commandArguments = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(TerminationState::WIN, commandName, commandArguments);
    }
  }

  auto loseNode = terminationNode["Lose"];
  if (loseNode.IsDefined()) {
    spdlog::debug("Parsing lose conditions.");
    for (std::size_t c = 0; c < loseNode.size(); c++) {
      auto commandIt = loseNode[c].begin();
      auto commandName = commandIt->first.as<std::string>();
      auto commandArguments = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(TerminationState::LOSE, commandName, commandArguments);
    }
  }

  auto endNode = terminationNode["End"];
  if (endNode.IsDefined()) {
    spdlog::debug("Parsing end conditions.");
    for (std::size_t c = 0; c < endNode.size(); c++) {
      auto commandIt = endNode[c].begin();
      auto commandName = commandIt->first.as<std::string>();
      auto commandArguments = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(TerminationState::NONE, commandName, commandArguments);
    }
  }
}

void GDYFactory::parseGlobalVariables(YAML::Node variablesNode) {
  if (!variablesNode.IsDefined()) {
    return;
  }

  for (std::size_t p = 0; p < variablesNode.size(); p++) {
    auto variable = variablesNode[p];
    auto variableName = variable["Name"].as<std::string>();
    auto variableInitialValue = variable["InitialValue"].as<uint32_t>(0);
    globalVariableDefinitions_.insert({variableName, variableInitialValue});
  }
}

void GDYFactory::loadObjects(YAML::Node objects) {
  spdlog::info("Loading {0} objects...", objects.size());

  for (std::size_t i = 0; i < objects.size(); i++) {
    auto object = objects[i];
    auto objectName = object["Name"].as<std::string>();
    auto mapChar = object["MapCharacter"].as<char>('\0');
    auto observerDefinitions = object["Observers"];

    if (observerDefinitions.IsDefined()) {
      parseSpriteObserverDefinitions(objectName, observerDefinitions["Sprite2D"]);
      parseBlockObserverDefinitions(objectName, observerDefinitions["Block2D"]);
    }

    auto variables = object["Variables"];
    std::unordered_map<std::string, uint32_t> variableDefinitions;

    if (variables.IsDefined()) {
      for (std::size_t p = 0; p < variables.size(); p++) {
        auto variable = variables[p];
        auto variableName = variable["Name"].as<std::string>();
        auto variableInitialValue = variable["InitialValue"].as<uint32_t>(0);
        variableDefinitions.insert({variableName, variableInitialValue});
      }
    }

    uint32_t zIdx = 0;
    auto objectZIdx = object["Z"];
    if (objectZIdx.IsDefined()) {
      zIdx = objectZIdx.as<uint32_t>();
    }

    objectGenerator_->defineNewObject(objectName, zIdx, mapChar, variableDefinitions);

    auto initialActionsNode = object["InitialActions"];

    if (initialActionsNode.IsDefined()) {
      for (std::size_t a = 0; a < initialActionsNode.size(); a++) {
        auto initialActionNode = initialActionsNode[a];
        auto actionName = initialActionNode["Action"].as<std::string>();
        auto actionId = initialActionNode["ActionId"].as<uint32_t>(0);
        auto delay = initialActionNode["Delay"].as<uint32_t>(0);
        auto randomize = initialActionNode["Randomize"].as<bool>(false);

        objectGenerator_->addInitialAction(objectName, actionName, actionId, delay, randomize);
      }
    }
  }
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
  SpriteDefinition spriteDefinition;

  spriteDefinition.images = singleOrListNodeToList(spriteNode["Image"]);

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
                                                              std::vector<std::unordered_map<std::string, BehaviourCommandArguments>> actionPreconditions,
                                                              std::unordered_map<std::string, BehaviourCommandArguments> conditionalCommands) {
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

  // If the object is _empty do nothing
  if (objectName == "_empty") {
    return;
  }

  // Get preconditions
  std::vector<std::unordered_map<std::string, BehaviourCommandArguments>> actionPreconditions;

  if (preconditionsNode.IsDefined()) {
    for (std::size_t c = 0; c < preconditionsNode.size(); c++) {
      auto preconditionsIt = preconditionsNode[c].begin();
      auto preconditionCommandName = preconditionsIt->first.as<std::string>();
      auto preconditionCommandArgumentsNode = preconditionsIt->second;

      auto preconditionCommandArgumentMap = singleOrListNodeToCommandArguments(preconditionCommandArgumentsNode);

      actionPreconditions.push_back({{preconditionCommandName, preconditionCommandArgumentMap}});
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

  for (std::size_t c = 0; c < commandsNode.size(); c++) {
    auto commandIt = commandsNode[c].begin();
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
    std::vector<std::unordered_map<std::string, BehaviourCommandArguments>> actionPreconditions) {
  if (commandNode.IsMap()) {
    // TODO: don't really like this check being done here. should be pushed into the object class really?
    if (commandName == "exec") {
      // We have an execute action that we need to parse slightly differently

      BehaviourCommandArguments commandArgumentMap;

      for (YAML::const_iterator execArgNode = commandNode.begin(); execArgNode != commandNode.end(); ++execArgNode) {
        auto execArgName = execArgNode->first.as<std::string>();
        auto execArgValue = execArgNode->second.as<std::string>();

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

      std::unordered_map<std::string, BehaviourCommandArguments> parsedSubCommands;
      for (std::size_t sc = 0; sc < conditionSubCommands.size(); sc++) {
        auto subCommandIt = conditionSubCommands[sc].begin();
        auto subCommandName = subCommandIt->first.as<std::string>();
        auto subCommandArguments = subCommandIt->second;

        auto subCommandArgumentMap = singleOrListNodeToCommandArguments(subCommandArguments);

        spdlog::debug("Parsing subcommand {0} conditions", subCommandName);

        parsedSubCommands.insert({subCommandName, subCommandArgumentMap});
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

void GDYFactory::loadActionInputsDefinition(std::string actionName, YAML::Node InputMappingNode) {
  spdlog::debug("Loading action mapping for action {0}", actionName);

  // Internal actions can only be called by using "exec" within other actions
  bool internal = InputMappingNode["Internal"].as<bool>(false);
  bool relative = InputMappingNode["Relative"].as<bool>(false);

  ActionInputsDefinition inputDefinition;
  inputDefinition.relative = relative;
  inputDefinition.internal = internal;

  auto inputMappingNode = InputMappingNode["Inputs"];
  if (!inputMappingNode.IsDefined()) {
    inputDefinition.inputMappings = defaultActionInputMappings();

  } else {
    for (YAML::const_iterator mappingNode = inputMappingNode.begin(); mappingNode != inputMappingNode.end(); ++mappingNode) {
      auto actionId = mappingNode->first.as<uint32_t>();

      InputMapping inputMapping;
      auto directionAndVector = mappingNode->second;

      auto vectorToDestNode = directionAndVector["VectorToDest"];
      if (vectorToDestNode.IsDefined()) {
        glm::ivec2 vector = {
            vectorToDestNode[0].as<int32_t>(0),
            vectorToDestNode[1].as<int32_t>(0)};

        inputMapping.vectorToDest = vector;
      }

      auto oreintationVectorNode = directionAndVector["OrientationVector"];
      if (oreintationVectorNode.IsDefined()) {
        glm::ivec2 vector = {
            oreintationVectorNode[0].as<int32_t>(0),
            oreintationVectorNode[1].as<int32_t>(0)};

        inputMapping.orientationVector = vector;
      }

      auto descriptionNode = directionAndVector["Description"];
      if (descriptionNode.IsDefined()) {
        inputMapping.description = descriptionNode.as<std::string>();
      }

      inputDefinition.inputMappings[actionId] = inputMapping;
    }
  }

  actionInputsDefinitions_[actionName] = inputDefinition;

  objectGenerator_->setActionInputDefinitions(actionInputsDefinitions_);
}

void GDYFactory::loadActions(YAML::Node actions) {
  spdlog::info("Loading {0} actions...", actions.size());
  for (std::size_t i = 0; i < actions.size(); i++) {
    auto action = actions[i];
    auto actionName = action["Name"].as<std::string>();
    auto behavioursNode = action["Behaviours"];

    loadActionInputsDefinition(actionName, action["InputMapping"]);

    for (std::size_t b = 0; b < behavioursNode.size(); b++) {
      auto behaviourNode = behavioursNode[b];
      auto srcNode = behaviourNode["Src"];
      auto dstNode = behaviourNode["Dst"];

      auto srcObjectNames = singleOrListNodeToList(srcNode["Object"]);
      auto dstObjectNames = singleOrListNodeToList(dstNode["Object"]);

      for (auto srcName : srcObjectNames) {
        parseActionBehaviours(ActionBehaviourType::SOURCE, srcName, actionName, dstObjectNames, srcNode["Commands"], srcNode["Preconditions"]);
      }

      for (auto dstName : dstObjectNames) {
        parseActionBehaviours(ActionBehaviourType::DESTINATION, dstName, actionName, srcObjectNames, dstNode["Commands"], EMPTY_NODE);
      }
    }
  }
}

std::shared_ptr<TerminationHandler> GDYFactory::createTerminationHandler(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Player>> players) const {
  return terminationGenerator_->newInstance(grid, players);
}

std::vector<std::string> GDYFactory::singleOrListNodeToList(YAML::Node singleOrList) {
  std::vector<std::string> values;
  if (singleOrList.IsScalar()) {
    values.push_back(singleOrList.as<std::string>());
  } else if (singleOrList.IsSequence()) {
    for (std::size_t s = 0; s < singleOrList.size(); s++) {
      values.push_back(singleOrList[s].as<std::string>());
    }
  }

  return values;
}

BehaviourCommandArguments GDYFactory::singleOrListNodeToCommandArguments(YAML::Node singleOrList) {
  BehaviourCommandArguments map;
  if (singleOrList.IsScalar()) {
    map["0"] = singleOrList;
  } else if (singleOrList.IsSequence()) {
    for (std::size_t s = 0; s < singleOrList.size(); s++) {
      map[std::to_string(s)] = singleOrList[s];
    }
  }

  return map;
}

std::unordered_map<uint32_t, InputMapping> GDYFactory::defaultActionInputMappings() const {
  std::unordered_map<uint32_t, InputMapping> defaultInputMappings{
      {1, InputMapping{{-1, 0}, {-1, 0}, "Left"}},
      {2, InputMapping{{0, -1}, {0, -1}, "Up"}},
      {3, InputMapping{{1, 0}, {1, 0}, "Right"}},
      {4, InputMapping{{0, 1}, {0, 1}, "Down"}}};

  return defaultInputMappings;
}

std::unordered_map<std::string, ActionInputsDefinition> GDYFactory::getActionInputsDefinitions() const {
  return actionInputsDefinitions_;
}

std::shared_ptr<TerminationGenerator> GDYFactory::getTerminationGenerator() const {
  return terminationGenerator_;
}

std::shared_ptr<LevelGenerator> GDYFactory::getLevelGenerator() const {
  return mapReaderLevelGenerator_;
}

std::shared_ptr<ObjectGenerator> GDYFactory::getObjectGenerator() const {
  return objectGenerator_;
}

std::unordered_map<std::string, SpriteDefinition> GDYFactory::getSpriteObserverDefinitions() const {
  return spriteObserverDefinitions_;
}

std::unordered_map<std::string, BlockDefinition> GDYFactory::getBlockObserverDefinitions() const {
  return blockObserverDefinitions_;
}

std::unordered_map<std::string, int32_t> GDYFactory::getGlobalVariableDefinitions() const {
  return globalVariableDefinitions_;
}

PlayerObserverDefinition GDYFactory::getPlayerObserverDefinition() const {
  return playerObserverDefinition_;
}

std::string GDYFactory::getAvatarObject() const {
  return avatarObject_;
}

void GDYFactory::overrideTileSize(uint32_t tileSize) {
  tileSize_ = tileSize;
}

uint32_t GDYFactory::getTileSize() const {
  return tileSize_;
}

uint32_t GDYFactory::getNumLevels() const {
  return levelStrings_.size();
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