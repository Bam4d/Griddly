
#include <unordered_map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/Grid.cpp"
#include "Griddy/Core/Grid.hpp"
#include "Griddy/Core/Objects/Object.cpp"
#include "Griddy/Core/Objects/Object.hpp"

#include "Griddy/Core/Objects/MockObject.cpp"

namespace griddy {

TEST(GridTest, getHeightAndWidth) {
  Grid grid(123, 456);

  ASSERT_EQ(grid.getWidth(), 123);
  ASSERT_EQ(grid.getHeight(), 456);
}

TEST(GridTest, initializeObject) {
  Grid grid(123, 456);

  auto mockObject = std::shared_ptr<griddy::MockObject>(new griddy::MockObject());

  ASSERT_EQ(grid.getObjects().size(), 0);

  grid.initObject({1, 2}, mockObject);

  ASSERT_EQ(grid.getObject({1, 2}), mockObject);
  ASSERT_EQ(grid.getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwice) {
  Grid grid(123, 456);

  auto mockObject = std::shared_ptr<griddy::MockObject>(new griddy::MockObject());
  auto mockObject2 = std::shared_ptr<griddy::MockObject>(new griddy::MockObject());

  ASSERT_EQ(grid.getObjects().size(), 0);

  grid.initObject({1, 2}, mockObject);
  grid.initObject({1, 2}, mockObject2);

  // The second init should be ignored because it is in the same location as the
  // first object
  ASSERT_EQ(grid.getObject({1, 2}), mockObject);
  ASSERT_EQ(grid.getObjects().size(), 1);
}

TEST(GridTest, initializeObjectTwice) {
  Grid grid(123, 456);

  auto mockObject = std::shared_ptr<griddy::MockObject>(new griddy::MockObject());

  ASSERT_EQ(grid.getObjects().size(), 0);

  grid.initObject({1, 2}, mockObject);
  grid.initObject({4, 4}, mockObject);

  // There second init should be ignored because the first one is the same
  // object
  ASSERT_EQ(grid.getObject({1, 2}), mockObject);
  ASSERT_EQ(grid.getObject({4, 4}), nullptr);
  ASSERT_EQ(grid.getObjects().size(), 1);
}

}  // namespace griddy