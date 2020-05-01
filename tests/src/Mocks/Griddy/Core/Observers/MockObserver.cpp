#include "Griddy/Core/Observers/Observer.hpp"
#include "gmock/gmock.h"

namespace griddy {
class MockObserver : public Observer {
 public:
  MockObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}
  ~MockObserver() {}

  MOCK_METHOD(std::unique_ptr<uint8_t[]>, update, (int playerId), (const));
  MOCK_METHOD(std::unique_ptr<uint8_t[]>, reset, (), (const));

  MOCK_METHOD(std::vector<uint32_t>, getShape, (), (const));
  MOCK_METHOD(std::vector<uint32_t>, getStrides, (), (const));
};
}  // namespace griddy