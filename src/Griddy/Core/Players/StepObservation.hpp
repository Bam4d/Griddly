#include <memory>

namespace griddy {

class Info;

struct StepObservation {
  std::unique_ptr<uint8_t[]> observation;
  int reward;
  std::unique_ptr<Info> info;
};

struct Info {
};
}  // namespace griddy