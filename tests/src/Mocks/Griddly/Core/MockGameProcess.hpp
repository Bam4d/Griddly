#pragma once
#include <vector>
#include "Griddly/Core/GameProcess.hpp"

#include "gmock/gmock.h"

namespace griddly {
class MockGameProcess : public GameProcess {
 public:
  MockGameProcess() : GameProcess(ObserverType::NONE, nullptr, nullptr) {}
  ~MockGameProcess() {}

  MOCK_METHOD(std::shared_ptr<uint8_t>, observe, (uint32_t playerId), (const));
  MOCK_METHOD(ActionResult, performActions, (uint32_t playerId, std::vector<std::shared_ptr<Action>> actions, bool updateTicks), ());

  MOCK_METHOD(void, init, (), ());

  MOCK_METHOD(std::shared_ptr<uint8_t>, reset, (), (const));

  MOCK_METHOD(std::string, getProcessName, (), (const));

  MOCK_METHOD(void, addPlayer, (std::shared_ptr<Player>), ());
};
}  // namespace griddly