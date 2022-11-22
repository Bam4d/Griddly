#pragma once

#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"

namespace griddly {

struct ASCIIObserverConfig : public ObserverConfig {
  uint32_t asciiPadWidth = 4;
  bool includePlayerId = false;
};

class ASCIIObserver : public Observer, public TensorObservationInterface {
 public:
  explicit ASCIIObserver(std::shared_ptr<Grid> grid, ASCIIObserverConfig& observerConfig);
  ~ASCIIObserver() override = default;

  void init(std::vector<std::weak_ptr<Observer>> playerObservers) override;

  uint8_t& update() override;
  void reset(std::shared_ptr<Object> avatarObject = nullptr) override;
  void resetShape() override;

  ObserverType getObserverType() const override;

 protected:
  void renderLocation(glm::ivec2 objectLocation, glm::ivec2 outputLocation, bool resetLocation = false) const;

 private:
  std::shared_ptr<uint8_t> observation_;
  uint32_t observationChannels_;
  uint32_t channelsBeforePlayerCount_;
  uint32_t channelsBeforeRotation_;
  uint32_t channelsBeforeVariables_;

  ASCIIObserverConfig config_;
};

}  // namespace griddly
