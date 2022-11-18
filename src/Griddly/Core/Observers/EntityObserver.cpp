#include "EntityObserver.hpp"
namespace griddly {

EntityObserver::EntityObserver(std::shared_ptr<Grid> grid, EntityObserverConfig& config) : Observer(std::move(grid), config) {
  config_ = config;
}

void EntityObserver::init(std::vector<std::weak_ptr<Observer>> playerObservers) {
  Observer::init(playerObservers);

  const auto& actionInputsDefinitions = config_.actionInputsDefinitions;
  for (const auto& actionInputDefinition : actionInputsDefinitions) {
    if (actionInputDefinition.second.internal) {
      internalActions_.insert(actionInputDefinition.first);
    }
  }

  if (config_.globalVariableMapping.size() > 0) {
    entityFeatures_.insert({"__global__", config_.globalVariableMapping});
  }

  // Precalclate offsets for entity configurations
  for (const auto& objectName : config_.objectNames) {
    spdlog::debug("Creating entity config and features for entity {0}", objectName);

    std::vector<std::string> featureNames{"x", "y", "z"};
    EntityConfig entityConfig;
    auto includeRotation = config_.includeRotation.find(objectName) != config_.includeRotation.end();
    auto includePlayerId = config_.includePlayerId.find(objectName) != config_.includePlayerId.end();

    if (includeRotation) {
      entityConfig.rotationOffset = entityConfig.totalFeatures;
      entityConfig.totalFeatures += 2;

      featureNames.push_back("ox");
      featureNames.push_back("oy");
    }

    if (includePlayerId) {
      entityConfig.playerIdOffset = entityConfig.totalFeatures;
      entityConfig.totalFeatures += 1;
      featureNames.push_back("playerId");
    }

    const auto& entityVariableMap = config_.entityVariableMapping[objectName];

    if (entityVariableMap.size() > 0) {
      entityConfig.variableOffset = entityConfig.totalFeatures;
      entityConfig.variableNames.insert(entityConfig.variableNames.end(), entityVariableMap.begin(), entityVariableMap.end());
      entityConfig.totalFeatures += entityVariableMap.size();

      featureNames.insert(featureNames.end(), entityVariableMap.begin(), entityVariableMap.end());
    }

    entityFeatures_.insert({objectName, featureNames});
    entityConfig_.insert({objectName, entityConfig});
  }
}

const std::unordered_map<std::string, std::vector<std::string>>& EntityObserver::getEntityVariableMapping() const {
  return config_.entityVariableMapping;
}

const std::unordered_map<std::string, std::vector<std::string>>& EntityObserver::getEntityFeatures() const {
  return entityFeatures_;
}

void EntityObserver::reset(std::shared_ptr<Object> avatarObject) {
  Observer::reset(avatarObject);

  // there are no additional steps until this observer can be used.
  observerState_ = ObserverState::READY;
}

EntityObservations& EntityObserver::update() {
  buildObservations(entityObservations_);

  if (config_.includeMasks) {
    buildMasks(entityObservations_);
  }

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

glm::ivec2 EntityObserver::resolveLocation(const glm::ivec2& location) const {
  const auto& observableGrid = getObservableGrid();

  auto resolvedLocation = location - glm::ivec2{observableGrid.left, observableGrid.bottom};

  if (doTrackAvatar_) {
    const auto& avatarDirection = avatarObject_->getObjectOrientation().getDirection();

    if (config_.rotateWithAvatar) {
      switch (avatarDirection) {
        default:
        case Direction::UP:
        case Direction::NONE:
          // TODO: dont need to do anything here
          break;
        case Direction::LEFT:
          resolvedLocation = {(gridWidth_ - 1) - resolvedLocation.y, resolvedLocation.x};
          break;
        case Direction::DOWN:
          resolvedLocation = {(gridWidth_ - 1) - resolvedLocation.x, (gridHeight_ - 1) - resolvedLocation.y};
          break;
        case Direction::RIGHT:
          resolvedLocation = {resolvedLocation.y, (gridHeight_ - 1) - resolvedLocation.x};
          break;
      }
    }
  }
  return resolvedLocation;
}

void EntityObserver::buildObservations(EntityObservations& entityObservations) {
  entityObservations.observations.clear();
  entityObservations.locations.clear();
  entityObservations.ids.clear();

  const auto& observableGrid = getObservableGrid();

  // Build global entity
  if (config_.globalVariableMapping.size() > 0) {
    std::vector<float> globalFeatureVector(config_.globalVariableMapping.size());
    uint32_t featureIdx = 0;
    const auto& globalVariables = grid_->getGlobalVariables();
    for (const auto& globalVariableName : config_.globalVariableMapping) {
      const auto& globalVariableValues = globalVariables.at(globalVariableName);
      if (globalVariableValues.size() == 1) {
        globalFeatureVector[featureIdx++] = static_cast<float>(*globalVariableValues.at(0));
      } else {
        globalFeatureVector[featureIdx++] = static_cast<float>(*globalVariableValues.at(config_.playerId));
      }
    }

    entityObservations.observations["__global__"].push_back(globalFeatureVector);
    entityObservations.ids["__global__"].push_back(0);
  }

  for (const auto& object : grid_->getObjects()) {
    const auto& name = object->getObjectName();
    auto location = object->getLocation();

    if (!(location.x < observableGrid.left || location.x > observableGrid.right || location.y < observableGrid.bottom || location.y > observableGrid.top)) {
      auto dy = *object->getObjectOrientation().getDy();
      auto dx = *object->getObjectOrientation().getDx();
      auto objectPlayerId = getEgocentricPlayerId(object->getPlayerId());
      auto zIdx = object->getZIdx();

      glm::ivec2 resolvedLocation = resolveLocation(location);

      spdlog::debug("Adding entity {0} to location ({1},{2})", name, resolvedLocation.x, resolvedLocation.y);

      const auto& entityConfig = entityConfig_.at(name);

      std::vector<float> featureVector(entityConfig.totalFeatures);
      featureVector[0] = static_cast<float>(resolvedLocation.x);
      featureVector[1] = static_cast<float>(resolvedLocation.y);
      featureVector[2] = static_cast<float>(zIdx);

      if (entityConfig.rotationOffset > 0) {
        featureVector[3] = static_cast<float>(dx);
        featureVector[4] = static_cast<float>(dy);
      }

      if (entityConfig.playerIdOffset > 0) {
        featureVector[entityConfig.playerIdOffset] = static_cast<float>(objectPlayerId);
      }

      for (uint32_t i = 0; i < entityConfig.variableNames.size(); i++) {
        auto variableValue = *object->getVariableValue(entityConfig.variableNames[i]);
        featureVector[entityConfig.variableOffset + i] = static_cast<float>(variableValue);
      }

      entityObservations.observations[name].push_back(featureVector);
      auto hash = std::hash<std::shared_ptr<Object>>()(object);
      entityObservations.ids[name].push_back(hash);
      entityObservations.locations[hash] = {static_cast<uint32_t>(resolvedLocation.x), static_cast<uint32_t>(resolvedLocation.y)};
    }
  }
}

void EntityObserver::buildMasks(EntityObservations& entityObservations) {
  std::unordered_set<std::string> allAvailableActionNames{};

  entityObservations.actorIds.clear();
  entityObservations.actorMasks.clear();

  for (const auto& actionNamesAtLocation : getAvailableActionNames(config_.playerId)) {
    auto location = actionNamesAtLocation.first;
    auto actionNames = actionNamesAtLocation.second;

    auto locationVec = glm::ivec2{location[0], location[1]};

    const auto& observableGrid = getObservableGrid();

    if (!(locationVec.x < observableGrid.left || locationVec.x > observableGrid.right || locationVec.y < observableGrid.bottom || locationVec.y > observableGrid.top)) {
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

  spdlog::debug("Getting available actionIds for action [{0}] at location [{1},{2}]", actionName, location.x, location.y);

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