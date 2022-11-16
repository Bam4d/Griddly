#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"
#include "ObserverConfigInterface.hpp"

namespace griddly {

class NoneObserver : public Observer, public TensorObservationInterface {
 public:
  explicit NoneObserver(std::shared_ptr<Grid> grid, std::vector<std::shared_ptr<Observer>> playerObservers);
  ~NoneObserver() override = default;

  uint8_t& update() override;
  void resetShape() override;

  ObserverType getObserverType() const override;

 private:
  std::shared_ptr<uint8_t> emptyObs_;
};

}  // namespace griddly