#include "../Grid.hpp"
#include "Observer.hpp"
#include "TensorObservationInterface.hpp"
#include "ObserverConfigInterface.hpp"

namespace griddly {

class NoneObserver : public Observer, public TensorObservationInterface {
 public:
  explicit NoneObserver(std::shared_ptr<Grid> grid);
  ~NoneObserver() override = default;

  const DLTensor& update() override;
  const DLTensor& getObservationTensor() override;
  
  void resetShape() override;

  ObserverType getObserverType() const override;

 private:
  std::shared_ptr<uint8_t> emptyObs_;


  DLTensor emptyTensor_;
  std::vector<int64_t> emptyTensorShape_;
  std::vector<int64_t> emptyTensorStrides_;
};

}  // namespace griddly