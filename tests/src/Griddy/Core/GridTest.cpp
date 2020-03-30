
#include <unordered_map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/Grid.cpp"
#include "Griddy/Core/Objects/Object.cpp"

#include "../../Mocks/Griddy/Core/Objects/MockObject.cpp"

using ::testing::Return;

namespace griddy {

TEST(GridTest, getHeightAndWidth) {
  auto grid = std::shared_ptr<Grid>(new Grid(123, 456));

  ASSERT_EQ(grid->getWidth(), 123);
  ASSERT_EQ(grid->getHeight(), 456);
}

TEST(GridTest, initializeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid(123, 456));

  auto mockObject = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject({1, 2}, mockObject);

  ASSERT_EQ(grid->getObject({1, 2}), mockObject);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectPositionTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid(123, 456));

  auto mockObject = std::shared_ptr<Object>(new MockObject());
  auto mockObject2 = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject({1, 2}, mockObject);
  grid->initObject({1, 2}, mockObject2);

  // The second init should be ignored because it is in the same location as the
  // first object
  ASSERT_EQ(grid->getObject({1, 2}), mockObject);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, initializeObjectTwice) {
  auto grid = std::shared_ptr<Grid>(new Grid(123, 456));

  auto mockObject = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  grid->initObject({1, 2}, mockObject);
  grid->initObject({4, 4}, mockObject);

  // There second init should be ignored because the first one is the same
  // object
  ASSERT_EQ(grid->getObject({1, 2}), mockObject);
  ASSERT_EQ(grid->getObject({4, 4}), nullptr);
  ASSERT_EQ(grid->getObjects().size(), 1);
}

TEST(GridTest, removeObject) {
  auto grid = std::shared_ptr<Grid>(new Grid(123, 456));

  auto mockObject = std::shared_ptr<MockObject>(new MockObject());

  auto objectLocation = GridLocation(1, 2);

  EXPECT_CALL(*mockObject, getLocation())
      .Times(1)
      .WillOnce(Return(objectLocation));

  grid->initObject(objectLocation, mockObject);

  ASSERT_EQ(grid->removeObject(mockObject), true);
  ASSERT_EQ(grid->getObject(objectLocation), nullptr);
  ASSERT_EQ(grid->getObjects().size(), 0);
}

TEST(GridTest, removeObjectNotInitialized) {
  auto grid = std::shared_ptr<Grid>(new Grid(123, 456));

  auto mockObject = std::shared_ptr<Object>(new MockObject());

  ASSERT_EQ(grid->getObjects().size(), 0);

  ASSERT_EQ(grid->removeObject(mockObject), false);
}

}  // namespace griddy