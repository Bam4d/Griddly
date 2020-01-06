
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Griddy/Core/Objects/Object.cpp"
#include "Griddy/Core/Objects/Object.hpp"
#include "Griddy/Core/Grid.cpp"
#include "Griddy/Core/Grid.hpp"

namespace griddy {
TEST(GridTest, getHeightAndWidth) {
  Grid grid(123, 456);

  ASSERT_EQ(grid.getWidth(), 123);
  ASSERT_EQ(grid.getHeight(), 456);
}
}