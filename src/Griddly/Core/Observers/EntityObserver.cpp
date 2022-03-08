#include "EntityObserver.hpp"
namespace griddly {

EntityObserver::EntityObserver(std::shared_ptr<Grid> grid) : Observer(std::move(grid)) {
}

void EntityObserver::init(EntityObserverConfig& config) {
  Observer::init(config);
  config_ = config;

  const auto& actionInputsDefinitions = config_.actionInputsDefinitions;
  for (const auto& actionInputDefinition : actionInputsDefinitions) {
    if (actionInputDefinition.second.internal) {
      internalActions_.insert(actionInputDefinition.first);
    }
  }
}

const EntityObserverConfig& EntityObserver::getConfig() const {
  return config_;
}

const std::unordered_map<std::string, std::vector<std::string>>& EntityObserver::getEntityVariableMapping() const {
  return config_.entityVariableMapping;
}

void EntityObserver::reset() {
  Observer::reset();

  // there are no additional steps until this observer can be used.
  observerState_ = ObserverState::READY;
}

EntityObservations& EntityObserver::update() {
  buildObservations(entityObservations_);
  buildMasks(entityObservations_);

  grid_->purgeUpdatedLocations(config_.playerId);

  return entityObservations_;
}

void EntityObserver::resetShape() {
  gridWidth_ = config_.overrideGridWidth > 0 ? config_.overrideGridWidth : grid_->getWidth();
  gridHeight_ = config_.overrideGridHeight > 0 ? config_.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();
}

ObserverType EntityObserver::getObserverType() const {
  return ObserverType::ENTITY;
}

void EntityObserver::buildObservations(EntityObservations& entityObservations) {
  entityObservations.observations.clear();
  entityObservations.locations.clear();
  entityObservations.ids.clear();

  for (const auto& object : grid_->getObjects()) {
    const auto& name = object->getObjectName();
    auto location = object->getLocation();
    auto orientationUnitVector = object->getObjectOrientation().getUnitVector();
    auto objectPlayerId = getEgocentricPlayerId(object->getPlayerId());
    auto zIdx = object->getZIdx();

    const auto& featureVariables = config_.entityVariableMapping[name];

    auto numVariables = featureVariables.size();
    auto numFeatures = 6 + numVariables;

    std::vector<float> featureVector(numFeatures);
    featureVector[0] = static_cast<float>(location[0]);
    featureVector[1] = static_cast<float>(location[1]);
    featureVector[2] = static_cast<float>(zIdx);
    featureVector[3] = static_cast<float>(orientationUnitVector.x);
    featureVector[4] = static_cast<float>(orientationUnitVector.y);
    featureVector[5] = static_cast<float>(objectPlayerId);
    for (uint32_t i = 0; i < numVariables; i++) {
      auto variableValue = *object->getVariableValue(featureVariables[i]);
      featureVector[6 + i] = static_cast<float>(variableValue);
    }

    entityObservations.observations[name].push_back(featureVector);
    auto hash = std::hash<std::shared_ptr<Object>>()(object);
    entityObservations.ids[name].push_back(hash);
    entityObservations.locations[hash] = {static_cast<uint32_t>(location.x), static_cast<uint32_t>(location.y)};
  }

}

void EntityObserver::buildMasks(EntityObservations& entityObservations) {

  std::unordered_set<std::string> allAvailableActionNames{};

  for (const auto& actionNamesAtLocation : getAvailableActionNames(config_.playerId)) {
    auto location = actionNamesAtLocation.first;
    auto actionNames = actionNamesAtLocation.second;

    auto locationVec = glm::ivec2{location[0], location[1]};

    for (const auto& actionName : actionNames) {
      spdlog::debug("[{0}] available at location [{1}, {2}]", actionName, location.x, location.y);

      auto actionInputsDefinitions = config_.actionInputsDefinitions;
      std::vector<uint32_t> mask(actionInputsDefinitions[actionName].inputMappings.size() + 1);
      mask[0] = 1;  // NOP is always available

      auto objectAtLocation = grid_->getObject(location);
      auto entityId = std::hash<std::shared_ptr<Object>>()(objectAtLocation);
      auto actionIdsForName = getAvailableActionIdsAtLocation(locationVec, actionName);

      for (auto id : actionIdsForName) {
        mask[id] = 1;
      }

      entityObservations.actorMasks[actionName].push_back(mask);
      entityObservations.actorIds[actionName].push_back(entityId);

      allAvailableActionNames.insert(actionName);
    }
  }

}

std::unordered_map<glm::ivec2, std::unordered_set<std::string>> EntityObserver::getAvailableActionNames(uint32_t playerId) const {
  std::unordered_map<glm::ivec2, std::unordered_set<std::string>> availableActionNames;

  // For every object in the grid return the actions that the object can perform
  // TODO: do not iterate over all the objects if we have avatars.
  for (const auto& object : grid_->getObjects()) {
    if (playerId == object->getPlayerId()) {
      auto actions = object->getAvailableActionNames();

      for (const auto& internalActionName : internalActions_) {
        actions.erase(internalActionName);
      }

      auto location = object->getLocation();
      if (!actions.empty()) {
        availableActionNames.insert({location, actions});
      }
    }
  }

  return availableActionNames;
}

std::vector<uint32_t> EntityObserver::getAvailableActionIdsAtLocation(glm::ivec2 location, std::string actionName) const {
  auto srcObject = grid_->getObject(location);

  spdlog::debug("Getting available actionIds for action [{}] at location [{0},{1}]", actionName, location.x, location.y);

  std::vector<uint32_t> availableActionIds{};
  if (srcObject) {
    const auto& actionInputDefinitions = config_.actionInputsDefinitions;
    const auto& actionInputDefinition = actionInputDefinitions.at(actionName);

    auto relativeToSource = actionInputDefinition.relative;

    for (const auto& inputMapping : actionInputDefinition.inputMappings) {
      auto actionId = inputMapping.first;
      auto mapping = inputMapping.second;

      auto metaData = mapping.metaData;

      // Create an fake action to test for availability (and not duplicate a bunch of code)
      auto potentialAction = std::make_shared<Action>(Action(grid_, actionName, 0, 0, metaData));
      potentialAction->init(srcObject, mapping.vectorToDest, mapping.orientationVector, relativeToSource);

      if (srcObject->isValidAction(potentialAction)) {
        availableActionIds.push_back(actionId);
      }
    }
  }

  return availableActionIds;
}
}  // namespace griddly