#include "JiddlyPlayerWrapper.hpp"

JiddlyPlayerWrapper::JiddlyPlayerWrapper(int playerId, std::string playerName, std::shared_ptr<griddly::Observer> observer, std::shared_ptr<griddly::GDYFactory> gdyFactory, std::shared_ptr<griddly::GameProcess> gameProcess)
    : player_(std::make_shared<griddly::Player>(griddly::Player(playerId, playerName, observer, gameProcess))), gdyFactory_(gdyFactory), gameProcess_(gameProcess) {
}

std::shared_ptr<griddly::Player> JiddlyPlayerWrapper::unwrapped() {
  return player_;
}

e::val JiddlyPlayerWrapper::stepSingle(std::string actionName, std::vector<int32_t> actionArray, bool updateTicks) {
  if (gameProcess_ != nullptr && !gameProcess_->isInitialized()) {
    throw std::invalid_argument("Cannot send player commands when game has not been initialized.");
  }

  auto action = buildAction(actionName, actionArray);

  griddly::ActionResult actionResult;
  if (action != nullptr) {
    actionResult = player_->performActions({action}, updateTicks);
  } else {
    actionResult = player_->performActions({}, updateTicks);
  }

  auto info = buildInfo(actionResult);

  auto js_result = e::val::object();
  js_result.set("terminated", actionResult.terminated);
  js_result.set("info", info);

  return js_result;
}

e::val JiddlyPlayerWrapper::buildInfo(griddly::ActionResult actionResult) {
  auto js_info = e::val::object();

  if (actionResult.terminated) {
    auto js_playerResults = e::val::object();

    for (auto playerRes : actionResult.playerStates) {
      std::string playerStatusString;
      switch (playerRes.second) {
        case griddly::TerminationState::WIN:
          playerStatusString = "Win";
          break;
        case griddly::TerminationState::LOSE:
          playerStatusString = "Lose";
          break;
        case griddly::TerminationState::NONE:
          playerStatusString = "";
          break;
      }

      if (playerStatusString.size() > 0) {
        js_playerResults.set(std::to_string(playerRes.first), playerStatusString);
      }
    }
    js_info.set("playerResults", js_playerResults);
  }

  return js_info;
}

std::shared_ptr<griddly::Action> JiddlyPlayerWrapper::buildAction(std::string actionName, std::vector<int32_t> actionArray) {
  const auto& actionInputsDefinition = gdyFactory_->findActionInputsDefinition(actionName);
  auto playerAvatar = player_->getAvatar();
  auto playerId = player_->getId();

  const auto& inputMappings = actionInputsDefinition.inputMappings;

  if (playerAvatar != nullptr) {
    auto actionId = actionArray[0];

    if (inputMappings.find(actionId) == inputMappings.end()) {
      return nullptr;
    }

    const auto& mapping = inputMappings.at(actionId);
    const auto& vectorToDest = mapping.vectorToDest;
    const auto& orientationVector = mapping.orientationVector;
    const auto& metaData = mapping.metaData;
    const auto& action = std::make_shared<griddly::Action>(griddly::Action(gameProcess_->getGrid(), actionName, playerId, 0, metaData));
    action->init(playerAvatar, vectorToDest, orientationVector, actionInputsDefinition.relative);

    return action;
  } else {
    glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};

    auto actionId = actionArray[2];

    if (inputMappings.find(actionId) == inputMappings.end()) {
      return nullptr;
    }

    const auto& mapping = inputMappings.at(actionId);
    const auto& vector = mapping.vectorToDest;
    const auto& orientationVector = mapping.orientationVector;
    const auto& metaData = mapping.metaData;
    glm::ivec2 destinationLocation = sourceLocation + vector;

    auto action = std::make_shared<griddly::Action>(griddly::Action(gameProcess_->getGrid(), actionName, playerId, 0, metaData));
    action->init(sourceLocation, destinationLocation);

    return action;
  }
}