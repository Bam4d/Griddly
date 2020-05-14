#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

#include "../Grid.hpp"
#include "../LevelGenerators/MapReader.hpp"
#include "../Observers/TileObserver.hpp"
#include "../TurnBasedGameProcess.hpp"
#include "GDYFactory.hpp"
#include "Objects/Object.hpp"

namespace griddy {

GDYFactory::GDYFactory(std::shared_ptr<ObjectGenerator> objectGenerator, std::shared_ptr<TerminationGenerator> terminationGenerator) : objectGenerator_(objectGenerator), terminationGenerator_(terminationGenerator) {
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

  parseGlobalParameters(environment["Parameters"]);
  parseTerminationConditions(environment["Termination"]);

  spdlog::info("Loaded {0} levels", levelStrings_.size());
}

void GDYFactory::parsePlayerDefinition(YAML::Node playerNode) {
  if (!playerNode.IsDefined()) {
    spdlog::debug("No player configuration node specified, assuming multi-player with selection control");
    playerMode_ = PlayerMode::MULTI;
    actionControlMode_ = ActionControlMode::SELECTION;
    return;
  }

  auto modeString = playerNode["Mode"].as<std::string>();
  if (modeString == "SINGLE") {
    spdlog::debug("Single player game detected");
    playerMode_ = PlayerMode::SINGLE;
  } else if (modeString == "MULTI") {
    spdlog::debug("Multi player game detected");
    playerMode_ = PlayerMode::MULTI;
  }

  auto actionsNode = playerNode["Actions"];
  if (!actionsNode.IsDefined()) {
    spdlog::debug("No action configuration node specified, assuming selection control");
    actionControlMode_ = ActionControlMode::SELECTION;
    return;
  }

  // If all actions control a single avatar type
  auto directControlNode = actionsNode["DirectControl"];
  if (directControlNode.IsDefined()) {
    actionControlMode_ = ActionControlMode::DIRECT;
    auto avatarObjectName = directControlNode.as<std::string>();
    objectGenerator_->setAvatarObject(avatarObjectName);
    spdlog::debug("Actions will directly control the object with name={0}", avatarObjectName);
  } else {
    spdlog::debug("Actions must be performed by selecting tiles on the grid.");
    actionControlMode_ = ActionControlMode::SELECTION;
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
      auto commandParameters = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(TerminationState::WIN, commandName, commandParameters);
    }
  }

  auto loseNode = terminationNode["Lose"];
  if (loseNode.IsDefined()) {
    spdlog::debug("Parsing lose conditions.");
    for (std::size_t c = 0; c < loseNode.size(); c++) {
      auto commandIt = loseNode[c].begin();
      auto commandName = commandIt->first.as<std::string>();
      auto commandParameters = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(TerminationState::LOSE, commandName, commandParameters);
    }
  }

  auto endNode = terminationNode["End"];
  if (endNode.IsDefined()) {
    spdlog::debug("Parsing end conditions.");
    for (std::size_t c = 0; c < endNode.size(); c++) {
      auto commandIt = endNode[c].begin();
      auto commandName = commandIt->first.as<std::string>();
      auto commandParameters = singleOrListNodeToList(commandIt->second);

      terminationGenerator_->defineTerminationCondition(TerminationState::NONE, commandName, commandParameters);
    }
  }
}

void GDYFactory::parseGlobalParameters(YAML::Node parametersNode) {
  if (!parametersNode.IsDefined()) {
    return;
  }

  std::unordered_map<std::string, uint32_t> parameterDefinitions;
  for (std::size_t p = 0; p < parametersNode.size(); p++) {
    auto param = parametersNode[p];
    auto paramName = param["Name"].as<std::string>();
    auto paramInitialValueNode = param["InitialValue"];
    auto paramInitialValue = paramInitialValueNode.IsDefined() ? paramInitialValueNode.as<uint32_t>() : 0;
    globalParameterDefinitions_.insert({paramName, paramInitialValue});
  }
}

void GDYFactory::loadObjects(YAML::Node objects) {
  spdlog::info("Loading {0} objects...", objects.size());

  for (std::size_t i = 0; i < objects.size(); i++) {
    auto object = objects[i];
    auto objectName = object["Name"].as<std::string>();

    char mapChar = 0;
    auto mapCharObject = object["MapCharacter"];
    if (mapCharObject.IsDefined()) {
      mapChar = object["MapCharacter"].as<char>();
    }
    auto observerDefinitions = object["Observers"];

    if (observerDefinitions.IsDefined()) {
      parseSpriteObserverDefinition(objectName, observerDefinitions["Sprite2D"]);
      parseBlockObserverDefinition(objectName, observerDefinitions["Block2D"]);
    }

    auto params = object["Parameters"];
    std::unordered_map<std::string, uint32_t> parameterDefinitions;

    if (params.IsDefined()) {
      for (std::size_t p = 0; p < params.size(); p++) {
        auto param = params[p];
        auto paramName = param["Name"].as<std::string>();
        auto paramInitialValueNode = param["InitialValue"];
        auto paramInitialValue = paramInitialValueNode.IsDefined() ? paramInitialValueNode.as<uint32_t>() : 0;

        parameterDefinitions.insert({paramName, paramInitialValue});
      }
    }

    uint32_t zIdx = 0;
    auto objectZIdx = object["Z"];
    if (objectZIdx.IsDefined()) {
      zIdx = objectZIdx.as<uint32_t>();
    }

    objectGenerator_->defineNewObject(objectName, zIdx, mapChar, parameterDefinitions);
  }
}

void GDYFactory::parseSpriteObserverDefinition(std::string objectName, YAML::Node spriteNode) {
  if (!spriteNode.IsDefined()) {
    return;
  }

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

  spriteObserverDefinitions_.insert({objectName, spriteDefinition});
}

void GDYFactory::parseBlockObserverDefinition(std::string objectName, YAML::Node blockNode) {
  if (!blockNode.IsDefined()) {
    return;
  }
  BlockDefinition blockDefinition;
  auto colorNode = blockNode["Color"];
  for (std::size_t c = 0; c < colorNode.size(); c++) {
    blockDefinition.color[c] = colorNode[c].as<float>();
  }
  blockDefinition.shape = blockNode["Shape"].as<std::string>();
  blockDefinition.scale = blockNode["Scale"].IsDefined() ? blockNode["Scale"].as<float>() : 1.0;

  blockObserverDefinitions_.insert({objectName, blockDefinition});
}

ActionBehaviourDefinition GDYFactory::makeBehaviourDefinition(ActionBehaviourType behaviourType,
                                                              std::string objectName,
                                                              std::string associatedObjectName,
                                                              std::string actionName,
                                                              std::string commandName,
                                                              std::vector<std::string> commandParameters,
                                                              std::unordered_map<std::string, std::vector<std::string>> conditionalCommands) {
  ActionBehaviourDefinition behaviourDefinition;
  behaviourDefinition.actionName = actionName;
  behaviourDefinition.behaviourType = behaviourType;
  behaviourDefinition.commandName = commandName;
  behaviourDefinition.commandParameters = commandParameters;
  behaviourDefinition.conditionalCommands = conditionalCommands;

  switch (behaviourType) {
    case ActionBehaviourType::SOURCE:
      behaviourDefinition.sourceObjectName = objectName;
      behaviourDefinition.destinationObjectName = associatedObjectName;
      break;
    case ActionBehaviourType::DESTINATION:
      behaviourDefinition.destinationObjectName = objectName;
      behaviourDefinition.sourceObjectName = associatedObjectName;
      break;
  }

  return behaviourDefinition;
}

void GDYFactory::parseActionBehaviours(ActionBehaviourType actionBehaviourType, std::string objectName, std::string actionName, std::vector<std::string> associatedObjectNames, YAML::Node commands) {
  spdlog::debug("Parsing {0} commands for action {1}, object {2}", commands.size(), actionName, objectName);
  for (std::size_t c = 0; c < commands.size(); c++) {
    auto commandIt = commands[c].begin();
    // iterate through keys
    auto commandName = commandIt->first.as<std::string>();
    auto commandParams = commandIt->second;

    if (commandParams.IsMap()) {
      auto conditionParams = commandParams["Params"];
      auto conditionSubCommands = commandParams["Cmd"];

      auto commandParamStrings = singleOrListNodeToList(conditionParams);

      std::unordered_map<std::string, std::vector<std::string>> parsedSubCommands;
      for (std::size_t sc = 0; sc < conditionSubCommands.size(); sc++) {
        auto subCommandIt = conditionSubCommands[sc].begin();
        auto subCommandName = subCommandIt->first.as<std::string>();
        auto subCommandParams = subCommandIt->second;

        auto subCommandParamStrings = singleOrListNodeToList(subCommandParams);

        parsedSubCommands.insert({subCommandName, subCommandParamStrings});
      }

      for (auto associatedObjectName : associatedObjectNames) {
        auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, objectName, associatedObjectName, actionName, commandName, commandParamStrings, parsedSubCommands);

        objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
      }

    } else if (commandParams.IsSequence() || commandParams.IsScalar()) {
      auto commandParamStrings = singleOrListNodeToList(commandParams);
      for (auto associatedObjectName : associatedObjectNames) {
        auto behaviourDefinition = makeBehaviourDefinition(actionBehaviourType, objectName, associatedObjectName, actionName, commandName, commandParamStrings, {});
        objectGenerator_->defineActionBehaviour(objectName, behaviourDefinition);
      }
    } else {
      throw std::invalid_argument(fmt::format("Badly defined command {0}", commandName));
    }
  }
}

void GDYFactory::loadActions(YAML::Node actions) {
  spdlog::info("Loading {0} actions...", actions.size());
  for (std::size_t i = 0; i < actions.size(); i++) {
    auto action = actions[i];
    auto actionName = action["Name"].as<std::string>();
    auto behaviours = action["Behaviours"];

    for (std::size_t b = 0; b < behaviours.size(); b++) {
      auto behaviour = behaviours[b];
      auto src = behaviour["Src"];
      auto dst = behaviour["Dst"];

      auto srcTypeNames = singleOrListNodeToList(src["Type"]);
      auto dstTypeNames = singleOrListNodeToList(dst["Type"]);

      for (auto srcName : srcTypeNames) {
        parseActionBehaviours(ActionBehaviourType::SOURCE, srcName, actionName, dstTypeNames, src["Cmd"]);
      }

      for (auto dstName : dstTypeNames) {
        parseActionBehaviours(ActionBehaviourType::DESTINATION, dstName, actionName, srcTypeNames, dst["Cmd"]);
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

std::unordered_map<std::string, int32_t> GDYFactory::getGlobalParameterDefinitions() const {
  return globalParameterDefinitions_;
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

ActionControlMode GDYFactory::getActionControlMode() const {
  return actionControlMode_;
}

PlayerMode GDYFactory::getPlayerMode() const {
  return playerMode_;
}

}  // namespace griddy