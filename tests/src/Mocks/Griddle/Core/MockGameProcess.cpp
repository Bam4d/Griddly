#include <vector>
#include "Griddle/Core/GameProcess.hpp"

#include "gmock/gmock.h"

namespace griddle {
class MockGameProcess : public GameProcess {
 public:
  MockGameProcess() : GameProcess(nullptr, nullptr, nullptr) {}
  ~MockGameProcess() {}

  MOCK_METHOD(std::unique_ptr<uint8_t[]>, observe, (uint32_t playerId), (const));
  MOCK_METHOD(ActionResult, performActions, (uint32_t playerId, std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(void, init, (), ());

  MOCK_METHOD(std::unique_ptr<uint8_t[]>, reset, (), (const));

  MOCK_METHOD(std::string, getProcessName, (), (const));

  MOCK_METHOD(void, addPlayer, (std::shared_ptr<Player>), ());
};
}  // namespace griddle