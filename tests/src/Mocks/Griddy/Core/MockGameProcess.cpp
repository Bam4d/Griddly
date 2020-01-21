#include <vector>
#include "Griddy/Core/GameProcess.hpp"
#include "gmock/gmock.h"

namespace griddy {
class MockGameProcess : public GameProcess {
 public:
  MockGameProcess() : GameProcess({}, nullptr, nullptr) {}
  ~MockGameProcess() {}

  MOCK_METHOD(std::unique_ptr<uint8_t[]>, observe, (), (const));
  MOCK_METHOD(std::vector<int>, performActions, (int playerId, std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(void, startGame, (), (const));
  MOCK_METHOD(void, endGame, (), (const));

  MOCK_METHOD(std::string, getProcessName, (), (const));
};
}  // namespace griddy