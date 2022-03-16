#pragma once

#include "../Grid.hpp"
#include "Observer.hpp"
#include "ObserverConfigInterface.hpp"
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
  std::unordered_map<std::string, std::vector<std::string>> entityVariableMapping{};
  std::unordered_map<std::string, ActionInputsDefinition> actionInputsDefinitions{};
};

class EntityObserver : public Observer, public ObservationInterface<EntityObservations>, public ObserverConfigInterface<EntityObserverConfig> {
 public:
  EntityObserver(std::shared_ptr<Grid> grid);
  ~EntityObserver() override = default;

  ObserverType getObserverType() const override;

  void init(EntityObserverConfig& observerConfig) override;
  const EntityObserverConfig& getConfig() const override;

  EntityObservations& update() override;
  void reset() override;

  const std::unordered_map<std::string, std::vector<std::string>>& getEntityVariableMapping() const;

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
};
}  // namespace griddly