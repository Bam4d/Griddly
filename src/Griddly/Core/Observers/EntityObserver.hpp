#pragma once

#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"

namespace griddly {

struct EntityObservations {
  std::unordered_map<std::string, std::vector<std::vector<float>>> observations{};
  std::unordered_map<size_t, std::array<uint32_t, 2>> locations{};
  std::unordered_map<std::string, std::vector<size_t>> ids{};

  std::map<std::string, std::vector<std::vector<uint32_t>>> actorMasks{};
  std::map<std::string, std::vector<size_t>> actorIds{};
};

struct EntityObserverConfig : public ObserverConfig {
  std::unordered_set<std::string> includePlayerId{};
  std::unordered_set<std::string> includeRotation{};
  std::unordered_map<std::string, std::vector<std::string>> entityVariableMapping{};
  std::vector<std::string> globalVariableMapping{};
  std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions{};
  std::vector<std::string> objectNames{};
  bool includeMasks = false;
};

struct EntityConfig {
  uint32_t totalFeatures = 3;
  uint32_t rotationOffset = 0;
  uint32_t playerIdOffset = 0;
  uint32_t variableOffset = 0;
  std::vector<std::string> variableNames;
};

class EntityObserver : public Observer, public ObservationInterface<EntityObservations> {
 public:
  EntityObserver(std::shared_ptr<Grid> grid, EntityObserverConfig& observerConfig);
  ~EntityObserver() override = default;

  ObserverType getObserverType() const override;

  void init(std::vector<std::shared_ptr<Observer>> playerObservers) override;

  EntityObservations& update() override;
  void reset(std::shared_ptr<Object> avatarObject = nullptr) override;

  const std::unordered_map<std::string, std::vector<std::string>>& getEntityVariableMapping() const;

  const std::unordered_map<std::string, std::vector<std::string>>& getEntityFeatures() const;

 protected:
  void resetShape() override;

 private:
  void buildObservations(EntityObservations& entityObservations);
  void buildMasks(EntityObservations& entityObservations);

  glm::ivec2 resolveLocation(const glm::ivec2& location) const;

  std::unordered_map<glm::ivec2, std::unordered_set<std::string>> getAvailableActionNames(uint32_t playerId) const;
  std::vector<uint32_t> getAvailableActionIdsAtLocation(glm::ivec2 location, std::string actionName) const;

  EntityObservations entityObservations_{};
  EntityObserverConfig config_{};

  std::unordered_set<std::string> internalActions_{};

  std::unordered_map<std::string, EntityConfig> entityConfig_{};

  std::unordered_map<std::string, std::vector<std::string>> entityFeatures_;
};
}  // namespace griddly