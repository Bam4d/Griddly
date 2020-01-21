#include "Griddy/Core/Players/Player.hpp"
#include "gmock/gmock.h"

namespace griddy {

class MockPlayer : public Player {
  MOCK_METHOD(void, act, (std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(std::unique_ptr<uint8_t[]>, observe, (), ());

  MOCK_METHOD(std::string, getName, (), (const));
  MOCK_METHOD(int, getId, (), (const));
};
}  // namespace griddy