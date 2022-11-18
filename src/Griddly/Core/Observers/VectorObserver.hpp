#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"

namespace griddly {

struct VectorObserverConfig : public ObserverConfig {
  // Config for VECTOR observers only
  bool includeVariables = false;
  bool includeRotation = false;
  bool includePlayerId = false;
  std::vector<std::string> globalVariableMapping{};
};

class VectorObserver : public Observer, public TensorObservationInterface {
 public:
  explicit VectorObserver(std::shared_ptr<Grid> grid, VectorObserverConfig& observerConfig);
  ~VectorObserver() override = default;

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
  uint32_t channelsBeforeGlobalVariables_;

  VectorObserverConfig config_;
};

}  // namespace griddly
