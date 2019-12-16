#include<memory>
#include<vector>

#include"StepResponse.hpp"

namespace griddy {

class GriddyRLInterface {
 public:
  virtual std::shared_ptr<std::vector<int>> getObservationShape();
  virtual std::shared_ptr<std::vector<int>> getActionShape();
  virtual StepResponse step(int action);
  virtual StepResponse reset();

};

}  // namespace griddy::grid