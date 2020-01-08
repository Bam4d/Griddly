
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/Objects/Object.cpp"
#include "Griddy/Core/Objects/Object.hpp"
#include "Griddy/Core/Grid.cpp"
#include "Griddy/Core/Grid.hpp"

#include "Griddy/Core/Objects/MockObject.cpp"

namespace griddy {


TEST(GridTest, getHeightAndWidth) {
  Grid grid(123, 456);

  ASSERT_EQ(grid.getWidth(), 123);
  ASSERT_EQ(grid.getHeight(), 456);
}

TEST(GridTest, initializeObject) {
  Grid grid(123, 456);

  std::shared_ptr<griddy::MockObject> mockObject = std::shared_ptr<griddy::MockObject>(new griddy::MockObject());

  grid.initObject({1,2}, mockObject);
}

}