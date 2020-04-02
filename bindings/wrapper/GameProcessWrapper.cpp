#include "../../src/Griddy/Core/TurnBasedGameProcess.hpp"

namespace griddy {
class Py_GameProcessWrapper {
 public:
  Py_GameProcessWrapper(std::shared_ptr<TurnBasedGameProcess> gameWrapper) : gameWrapper_(gameWrapper) {
  }

  std::shared_ptr<TurnBasedGameProcess> unwrapped() {
      return gameWrapper_;
  }

 private:
  const std::shared_ptr<TurnBasedGameProcess> gameWrapper_;
};
}  // namespace griddy