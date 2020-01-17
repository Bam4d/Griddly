#include "Griddy/Core/GameProcess.hpp"
#include "gmock/gmock.h

namespace griddy {
class MockGameProcess : public GameProcess {
 public:
  MOCK_METHOD(std::unique_ptr<uint8_t[]>, observe, (std::shared_ptr<Player> player), (const));

  MOCK_METHOD(void, startGame, (), (const));
  MOCK_METHOD(void, endGame, (), (const));

  MOCK_METHOD(std::string, getProcessName, (), (const));
};
}  // namespace griddy