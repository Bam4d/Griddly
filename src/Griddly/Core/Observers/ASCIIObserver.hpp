#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"
#include "ObserverConfigInterface.hpp"

namespace griddly {

struct ASCIIObserverConfig : public ObserverConfig {
  uint32_t asciiPadWidth = 4;
  bool includePlayerId = false;

  virtual ~ASCIIObserverConfig() = default;
};

class ASCIIObserver : public Observer, public TensorObservationInterface, public ObserverConfigInterface<ASCIIObserverConfig> {
 public:
  explicit ASCIIObserver(std::shared_ptr<Grid> grid);
  ~ASCIIObserver() override = default;

  void init(ASCIIObserverConfig& observerConfig) override;
  const ASCIIObserverConfig& getConfig() const override;

  uint8_t& update() override;
  void reset() override;
  void resetShape() override;

  ObserverType getObserverType() const override;

 protected:
  void renderLocation(glm::ivec2 objectLocation, glm::ivec2 outputLocation, bool resetLocation = false) const;

 private:
  std::shared_ptr<uint8_t> observation_;
  bool trackAvatar_;
  uint32_t observationChannels_;
  uint32_t channelsBeforePlayerCount_;
  uint32_t channelsBeforeRotation_;
  uint32_t channelsBeforeVariables_;

  ASCIIObserverConfig config_;
};

}  // namespace griddly