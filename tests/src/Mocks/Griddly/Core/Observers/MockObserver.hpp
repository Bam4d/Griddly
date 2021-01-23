#pragma once

#include "Griddly/Core/Observers/Observer.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockObserver : public Observer {
 public:
  MockObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}
  ~MockObserver() {}

  MOCK_METHOD(void, init, (ObserverConfig observerConfig), ());
  MOCK_METHOD(uint8_t*, update, (), (const));
  MOCK_METHOD(uint8_t*, reset, (), ());
  MOCK_METHOD(void, resetShape, (), ());

  MOCK_METHOD(std::vector<uint32_t>, getShape, (), (const));
  MOCK_METHOD(std::vector<uint32_t>, getStrides, (), (const));

  MOCK_METHOD(ObserverType, getObserverType, (), (const));
};
}  // namespace griddly