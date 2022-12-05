#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"

namespace griddly {

class NoneObserver : public Observer, public TensorObservationInterface {
 public:
  explicit NoneObserver(std::shared_ptr<Grid> grid, ObserverConfig& config);
  ~NoneObserver() override = default;

  std::shared_ptr<ObservationTensor>& update() override;
  std::shared_ptr<ObservationTensor>& getObservationTensor() override;

  std::vector<int64_t>& getShape() override;
  std::vector<int64_t>& getStrides() override;

  void resetShape() override;

  ObserverType getObserverType() const override;

 private:
  std::shared_ptr<uint8_t> emptyObs_;

  std::shared_ptr<ObservationTensor> emptyTensor_;
  std::vector<int64_t> emptyTensorShape_;
  std::vector<int64_t> emptyTensorStrides_;
};

}  // namespace griddly
