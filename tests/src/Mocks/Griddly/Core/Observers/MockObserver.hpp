#pragma once

#include "Griddly/Core/Observers/Observer.hpp"
#include "gmock/gmock.h"

namespace griddly {

template<class ObserverConfigType = ObserverConfig, class ObservationType = uint8_t>
class MockObserver : public Observer {
 public:
  MockObserver(std::shared_ptr<Grid> grid, ObserverConfigType& config) : Observer(grid, config) {}
  ~MockObserver() override = default;

  MOCK_METHOD(void, init, (std::vector<std::weak_ptr<Observer>> playerObservers), ());
  MOCK_METHOD(ObservationType&, update, (), ());
  MOCK_METHOD(void, reset, (), ());
  MOCK_METHOD(void, reset, (std::shared_ptr<Object> avatarObject), ());
  MOCK_METHOD(void, resetShape, (), ());
  MOCK_METHOD(PartialObservableGrid, getObservableGrid, (), (const));

  MOCK_METHOD(std::vector<uint32_t>, getShape, (), (const));
  MOCK_METHOD(std::vector<uint32_t>, getStrides, (), (const));

  MOCK_METHOD(ObserverType, getObserverType, (), (const));
};
}  // namespace griddly
