#pragma once

#include "Griddly/Core/Observers/Observer.hpp"
#include "gmock/gmock.h"

namespace griddly {

template<class ObserverConfigType = ObserverConfig, class ObservationType = uint8_t>
class MockObserver : public Observer {
 public:
  MockObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}
  ~MockObserver() = default;

  MOCK_METHOD(void, init, (ObserverConfigType& observerConfig), ());
  MOCK_METHOD(ObservationType&, update, (), ());
  MOCK_METHOD(void, reset, (), ());
  MOCK_METHOD(void, resetShape, (), ());

  MOCK_METHOD(std::vector<uint32_t>, getShape, (), (const));
  MOCK_METHOD(std::vector<uint32_t>, getStrides, (), (const));

  MOCK_METHOD(ObserverType, getObserverType, (), (const));
};
}  // namespace griddly