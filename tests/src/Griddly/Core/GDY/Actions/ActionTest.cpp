#include "Griddly/Core/GDY/Actions/Action.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

TEST(ActionTest, initActionLocations) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  std::shared_ptr<Action> action = std::shared_ptr<Action>(new Action(mockGridPtr, "testAction", 0, 0));

  action->init({0, 1}, {3, 4});

  ASSERT_EQ(action->getVectorToDest(), glm::ivec2(3, 3));
}

}  // namespace griddly