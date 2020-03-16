#include <memory>

namespace griddy {

class Info;

struct StepObservation {
  std::shared_ptr<uint8_t[]> observation;
  int reward;
  std::shared_ptr<Info> info;
};

struct Info {
};
}  // namespace griddy