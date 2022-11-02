#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"
#include "ObserverConfigInterface.hpp"
#include "dlpack.h"

namespace griddly {

struct VectorObserverConfig : public ObserverConfig {
  // Config for VECTOR observers only
  bool includeVariables = false;
  bool includeRotation = false;
  bool includePlayerId = false;
  std::vector<std::string> globalVariableMapping{};
};

class VectorObserver : public Observer, public TensorObservationInterface, public ObserverConfigInterface<VectorObserverConfig> {
 public:
  explicit VectorObserver(std::shared_ptr<Grid> grid);
  ~VectorObserver() override = default;

  void init(VectorObserverConfig& observerConfig) override;

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
  uint32_t channelsBeforeGlobalVariables_;

  VectorObserverConfig config_;

  DLTensor vectorTensor_;
  std::vector<int64_t> vectorTensorShape_;
  std::vector<int64_t> vectorTensorStrides_;
};

}  // namespace griddly
