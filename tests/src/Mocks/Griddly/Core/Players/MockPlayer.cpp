#include "Griddly/Core/Players/Player.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockPlayer : public Player {
 public:
  MockPlayer() : Player(0, "testPlayer", nullptr) {
  }

  MOCK_METHOD(void, act, (std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(std::shared_ptr<uint8_t>, observe, (), ());

  MOCK_METHOD(std::string, getName, (), (const));
  MOCK_METHOD(uint32_t, getId, (), (const));

  MOCK_METHOD(std::shared_ptr<int32_t>, getScore, (), (const));
};
}  // namespace griddly