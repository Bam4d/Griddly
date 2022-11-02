#pragma once

#include "../Grid.hpp"
#include "Observer.hpp"
#include "ObserverConfigInterface.hpp"
#include "TensorObservationInterface.hpp"

namespace griddly {

struct ASCIIObserverConfig : public ObserverConfig {
  uint32_t asciiPadWidth = 4;
  bool includePlayerId = false;
};

class ASCIIObserver : public Observer, public TensorObservationInterface, public ObserverConfigInterface<ASCIIObserverConfig> {
 public:
  explicit ASCIIObserver(std::shared_ptr<Grid> grid);
  ~ASCIIObserver() override = default;

  void init(ASCIIObserverConfig& observerConfig) override;

  const DLTensor& update() override;
  const DLTensor& getObservationTensor() override;

  void reset() override;
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

  DLTensor asciiTensor_;
  std::vector<int64_t> asciiTensorShape_;
  std::vector<int64_t> asciiTensorStrides_;
};

}  // namespace griddly
