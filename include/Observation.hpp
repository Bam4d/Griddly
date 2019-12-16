#include <memory>
#include <vector>
namespace griddy {
class Observation {
 public:
  virtual std::shared_ptr<std::vector<int>> getObservationShape();
};

}  // namespace griddy