#pragma once

#include "Griddly/Core/GameProcess.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockPlayer : public Player {
 public:
  MockPlayer() : Player(0, "testPlayer", nullptr, nullptr) {
  }

  MOCK_METHOD(uint8_t*, observe, (), ());

  MOCK_METHOD(std::string, getName, (), (const));
  MOCK_METHOD(uint32_t, getId, (), (const));

  MOCK_METHOD(std::shared_ptr<int32_t>, getScore, (), (const));

  MOCK_METHOD(ActionResult, performActions, (std::vector<std::shared_ptr<Action>> actions, bool updateTicks), ());

  MOCK_METHOD(void, reset, (), ());

  MOCK_METHOD(std::shared_ptr<Object>, getAvatar, (), ());
  MOCK_METHOD(void, setAvatar, (std::shared_ptr<Object> avatarObject), ());

  MOCK_METHOD(std::shared_ptr<GameProcess>, getGameProcess, (), (const));
  MOCK_METHOD(std::shared_ptr<Observer>, getObserver, (), (const));
};
}  // namespace griddly