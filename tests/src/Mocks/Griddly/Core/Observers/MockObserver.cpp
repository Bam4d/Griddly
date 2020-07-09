#include "Griddly/Core/Observers/Observer.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockObserver : public Observer {
 public:
  MockObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}
  ~MockObserver() {}

  MOCK_METHOD(void, init, (ObserverConfig observerConfig), ());
  MOCK_METHOD(std::shared_ptr<uint8_t>, update, (int playerId), (const));
  MOCK_METHOD(std::shared_ptr<uint8_t>, reset, (), ());

  MOCK_METHOD(std::vector<uint32_t>, getShape, (), (const));
  MOCK_METHOD(std::vector<uint32_t>, getStrides, (), (const));
};
}  // namespace griddly