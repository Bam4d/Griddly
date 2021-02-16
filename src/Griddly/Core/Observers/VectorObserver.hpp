#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddly {

class VectorObserver : public Observer {
 public:
  VectorObserver(std::shared_ptr<Grid> grid);
  ~VectorObserver() override;

  void init(ObserverConfig observerConfig) override;

  uint8_t* update() const override;
  uint8_t* reset() override;
  void resetShape() override;

  ObserverType getObserverType() const override;
  glm::ivec2 getTileSize() const override;

  void print(std::shared_ptr<uint8_t> observation) override;

 protected:
  void renderLocation(glm::ivec2 objectLocation, glm::ivec2 outputLocation, bool resetLocation=false) const;

 private:
  std::shared_ptr<uint8_t> observation_;
  bool trackAvatar_;
  uint32_t observationChannels_;
  uint32_t channelsBeforeRotation_;
  uint32_t channelsBeforeVariables_;
};

}  // namespace griddly