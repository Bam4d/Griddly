#include "Griddy/Core/Observers/Observer.hpp"
#include "gmock/gmock.h"

namespace griddy {
class MockObserver : public Observer {
 public:
  MockObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}
  ~MockObserver() {}

  MOCK_METHOD(std::unique_ptr<uint8_t[]>, observe, (int playerId), (const));

  MOCK_METHOD(std::vector<uint>, getShape, (), (const));
  MOCK_METHOD(std::vector<uint>, getStrides, (), (const));
};
}  // namespace griddy