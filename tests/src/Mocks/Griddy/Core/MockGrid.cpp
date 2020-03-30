#include "Griddy/Core/Grid.hpp"
#include "gmock/gmock.h"

namespace griddy {
class MockGrid : public Grid {
 public:
  MockGrid() : Grid(10,10) {}

  MOCK_METHOD(void, cloneState, (), ());
  MOCK_METHOD(void, update, (std::vector<std::shared_ptr<Action>> actions), ());

  MOCK_METHOD(int, getCurrentScore, (int playerId), (const));
  MOCK_METHOD(int, getResources, (int playerId), (const));

  MOCK_METHOD(int, getWidth, (), (const));
  MOCK_METHOD(int, getHeight, (), (const));

  MOCK_METHOD(void, initObject, (GridLocation location, std::shared_ptr<Object> object), ());
  MOCK_METHOD(bool, removeObject, (std::shared_ptr<Object> object), ());
  MOCK_METHOD(std::unordered_set<std::shared_ptr<Object>>&, getObjects, (), ());

  MOCK_METHOD(std::shared_ptr<Object>, getObject, (GridLocation location), (const));
};
}  // namespace griddy