#include <memory>

#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/Observers/EntityObserver.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "ObserverRTSTestData.hpp"
#include "ObserverTestData.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Pair;
using ::testing::Return;
using ::testing::ReturnRef;

namespace griddly {

std::string printEntity(const std::vector<float>& entity) {
  std::string entityStr = "[";
  for (uint32_t v = 0; v < entity.size(); v++) {
    entityStr += fmt::format("{0} ", entity[v]);
  }
  entityStr += "]";
  return entityStr;
}

bool entityExists(const std::vector<std::vector<float>>& entityList, const std::vector<float>& entity) {
  for (uint32_t i = 0; i < entityList.size(); i++) {
    spdlog::debug("Searching for {0}. Comparing against {1}", printEntity(entity), printEntity(entityList[i]));
    if (entity.size() == entityList[i].size()) {
      bool allCorrect = true;
      for (uint32_t y = 0; y < entity.size(); y++) {
        if (entityList[i][y] != entity[y]) {
          allCorrect = false;
          break;
        }
      }
      if (allCorrect) {
        spdlog::debug("Found!");
        return true;
      }
    }
  }

  return false;
}

void runEntityObserverTest(EntityObserverConfig observerConfig,
                           Direction avatarDirection,
                           EntityObservations& expectedEntityObservervations) {
  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection));

  std::shared_ptr<EntityObserver> entityObserver = std::make_shared<EntityObserver>(testEnvironment.mockGridPtr, observerConfig);

  observerConfig.objectNames = testEnvironment.mockSinglePlayerObjectNames;

  entityObserver->init({entityObserver});
  entityObserver->reset(testEnvironment.mockAvatarObjectPtr);

  const auto& updateEntityObservations = entityObserver->update();

  ASSERT_EQ(updateEntityObservations.observations.size(), expectedEntityObservervations.observations.size());

  for (const auto& expectedObservationsIt : expectedEntityObservervations.observations) {
    auto entityName = expectedObservationsIt.first;
    auto expectedObservations = expectedObservationsIt.second;

    auto updateObservations = updateEntityObservations.observations.at(entityName);
    auto updateIds = updateEntityObservations.observations.at(entityName);

    // there should be the same number of entities in ids and observations
    ASSERT_EQ(updateObservations.size(), expectedObservations.size());
    ASSERT_EQ(updateIds.size(), expectedObservations.size());

    for (auto i = 0; i < updateObservations.size(); i++) {
      ASSERT_TRUE(entityExists(expectedObservations, updateObservations[i]));

      auto expectedEntityLocation = glm::ivec2{updateObservations[i][0], updateObservations[i][1]};
      auto updateId = updateEntityObservations.ids.at(entityName)[i];

      if (entityName != "__global__") {
        ASSERT_EQ(updateEntityObservations.locations.at(updateId)[0], expectedEntityLocation[0]);
        ASSERT_EQ(updateEntityObservations.locations.at(updateId)[1], expectedEntityLocation[1]);
      }
    }
  }

  testEnvironment.verifyAndClearExpectations();
}

void runEntityObserverRTSTest(EntityObserverConfig observerConfig,
                              EntityObservations& expectedEntityObservervations) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<EntityObserver> entityObserver = std::make_shared<EntityObserver>(testEnvironment.mockGridPtr, observerConfig);

  observerConfig.objectNames = testEnvironment.mockRTSObjectNames;

  entityObserver->init({entityObserver});

  entityObserver->reset();

  const auto& updateEntityObservations = entityObserver->update();
  const auto& entityVariableMapping = entityObserver->getEntityVariableMapping();

  ASSERT_EQ(updateEntityObservations.observations.size(), expectedEntityObservervations.observations.size());

  for (const auto& expectedObservationsIt : expectedEntityObservervations.observations) {
    auto entityName = expectedObservationsIt.first;
    auto expectedObservations = expectedObservationsIt.second;

    auto updateObservations = updateEntityObservations.observations.at(entityName);
    auto updateIds = updateEntityObservations.observations.at(entityName);

    // there should be the same number of entities in ids and observations
    ASSERT_EQ(updateObservations.size(), expectedObservations.size());
    ASSERT_EQ(updateIds.size(), expectedObservations.size());

    for (auto i = 0; i < updateObservations.size(); i++) {
      ASSERT_TRUE(entityExists(expectedObservations, updateObservations[i]));

      auto expectedEntityLocation = glm::ivec2{updateObservations[i][0], updateObservations[i][1]};
      auto updateId = updateEntityObservations.ids.at(entityName)[i];

      ASSERT_EQ(updateEntityObservations.locations.at(updateId)[0], expectedEntityLocation[0]);
      ASSERT_EQ(updateEntityObservations.locations.at(updateId)[1], expectedEntityLocation[1]);
    }
  }

  testEnvironment.verifyAndClearExpectations();
}

TEST(EntityObserverTest, defaultObserverConfig) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      false, false};

  EntityObservations expectedEntityObservervations;
  // "x", "y", "z"
  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{3, 3, 0},
        {1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{1, 3, 0},
        {3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false, false};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      -1,
      false,
      false};

  EntityObservations expectedEntityObservervations;
  // "x", "y", "z", "ox", "oy", "player_id"
  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 2, 0},
        {1, 0, 0},
        {1, 1, 0}}},
      {"mo3",
       {{1, 2, 0},
        {3, 1, 0},
        {3, 0, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{3, 3, 0},
        {1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{1, 3, 0},
        {3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{3, 3, 0},
        {1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{1, 3, 0},
        {3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, -1}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{3, 3, 0},
        {1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{1, 3, 0},
        {3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{3, 1, 0},    // 3,1
        {1, 3, 0},    // 1,3
        {2, 3, 0}}},  // 2,3
      {"mo3",
       {{3, 3, 0},     // 3, 3
        {2, 1, 0},     // 2, 1
        {1, 1, 0}}}};  // 1, 1

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{1, 1, 0},
        {3, 3, 0},
        {3, 2, 0}}},
      {"mo3",
       {{3, 1, 0},
        {1, 2, 0},
        {1, 3, 0}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, -1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{1, 3, 0},    // 1,3
        {3, 1, 0},    // 3,1
        {2, 1, 0}}},  // 2,1
      {"mo3",
       {{1, 1, 0},     // 1,1
        {2, 3, 0},     // 2,3
        {3, 3, 0}}}};  // 3,3

  runEntityObserverTest(config, Direction::LEFT, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, 0}}},
      {"mo1",
       {{0, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 2, 0},
        {1, 0, 0},
        {1, 1, 0}}},
      {"mo3",
       {{1, 2, 0},
        {3, 1, 0},
        {3, 0, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_UP) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, -1}}},
      {"mo1",
       {{0, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 2, 0},
        {1, 0, 0},
        {1, 1, 0}}},
      {"mo3",
       {{1, 2, 0},
        {3, 1, 0},
        {3, 0, 0}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 2, 0},
        {1, 0, 0},
        {1, 1, 0}}},
      {"mo3",
       {{1, 2, 0},
        {3, 1, 0},
        {3, 0, 0}}}};

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 2, 0},
        {1, 0, 0},
        {1, 1, 0}}},
      {"mo3",
       {{1, 2, 0},
        {3, 1, 0},
        {3, 0, 0}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, -1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 2, 0},
        {1, 0, 0},
        {1, 1, 0}}},
      {"mo3",
       {{1, 2, 0},
        {3, 1, 0},
        {3, 0, 0}}}};

  runEntityObserverTest(config, Direction::LEFT, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, -1}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, -1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::LEFT, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, -1}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {1, 2, 0}}},
      {"mo3",
       {{3, 2, 0},
        {3, 1, 0}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 1, 0}}},
      {"mo3",
       {{2, 1, 0},
        {1, 1, 0}}}};

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{1, 1, 0},
        {3, 2, 0}}},
      {"mo3",
       {{3, 1, 0},
        {1, 2, 0}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  config.includeRotation = {"avatar"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, -1, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1}}},
      {"mo2",
       {{3, 1, 0},    // 3,1
        {2, 1, 0}}},  // 2,1
      {"mo3",
       {{1, 1, 0}}}};  // 3,3

  runEntityObserverTest(config, Direction::LEFT, expectedEntityObservervations);
}

TEST(EntityObserverTest, entityVariableMapping) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      false, false};

  config.includeRotation = {"mo3"};

  config.globalVariableMapping = {"lightingR", "lightingG", "lightingB"};

  config.entityVariableMapping = {
      {"mo2", {"health", "max_health"}},
      {"mo3", {"health", "max_health"}}};

  EntityObservations expectedEntityObservervations;

  // "x", "y", "z", "ox", "oy", "player_id"
  expectedEntityObservervations.observations = {
      {"__global__",
       {{50, 100, 100}}},
      {"avatar",
       {{2, 2, 0}}},
      {"mo1",
       {{0, 0, -1},
        {1, 0, -1},
        {2, 0, -1},
        {3, 0, -1},
        {4, 0, -1},
        {0, 1, -1},
        {4, 1, -1},
        {0, 2, -1},
        {4, 2, -1},
        {0, 3, -1},
        {4, 3, -1},
        {0, 4, -1},
        {1, 4, -1},
        {2, 4, -1},
        {3, 4, -1},
        {4, 4, -1}}},
      {"mo2",
       {{3, 3, 0, 100, 100},
        {1, 1, 0, 5, 10},
        {1, 2, 0, 10, 20}}},
      {"mo3",
       {{1, 3, 0, 0, 0, 7, 10},
        {3, 2, 0, 0, 0, 5, 10},
        {3, 1, 0, 0, 0, 4, 20}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityObservervations);
}

TEST(EntityObserverTest, multiPlayer_Outline_Player1) {
  EntityObserverConfig config = {5, 5};
  config.playerId = 1;
  config.playerCount = 3;

  config.includePlayerId = {"A", "B", "C"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"W",
       {{0, 0, 0},
        {1, 0, 0},
        {2, 0, 0},
        {3, 0, 0},
        {4, 0, 0},
        {0, 1, 0},
        {4, 1, 0},
        {0, 2, 0},
        {4, 2, 0},
        {0, 3, 0},
        {4, 3, 0},
        {0, 4, 0},
        {1, 4, 0},
        {2, 4, 0},
        {3, 4, 0},
        {4, 4, 0}}},
      {"A",
       {{1, 1, 0, 1},
        {1, 2, 0, 2},
        {1, 3, 0, 3}}},
      {"B",
       {{2, 1, 0, 1},
        {2, 2, 0, 2},
        {2, 3, 0, 3}}},
      {"C",
       {{3, 1, 0, 1},
        {3, 2, 0, 2},
        {3, 3, 0, 3}}}};

  runEntityObserverRTSTest(config, expectedEntityObservervations);
}

TEST(EntityObserverTest, multiPlayer_Outline_Player2) {
  EntityObserverConfig config = {5, 5};
  config.playerId = 2;
  config.playerCount = 3;

  config.includePlayerId = {"A", "B", "C"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"W",
       {{0, 0, 0},
        {1, 0, 0},
        {2, 0, 0},
        {3, 0, 0},
        {4, 0, 0},
        {0, 1, 0},
        {4, 1, 0},
        {0, 2, 0},
        {4, 2, 0},
        {0, 3, 0},
        {4, 3, 0},
        {0, 4, 0},
        {1, 4, 0},
        {2, 4, 0},
        {3, 4, 0},
        {4, 4, 0}}},
      {"A",
       {{1, 1, 0, 2},
        {1, 2, 0, 1},
        {1, 3, 0, 3}}},
      {"B",
       {{2, 1, 0, 2},
        {2, 2, 0, 1},
        {2, 3, 0, 3}}},
      {"C",
       {{3, 1, 0, 2},
        {3, 2, 0, 1},
        {3, 3, 0, 3}}}};

  runEntityObserverRTSTest(config, expectedEntityObservervations);
}

TEST(EntityObserverTest, multiPlayer_Outline_Player3) {
  EntityObserverConfig config = {5, 5};
  config.playerId = 3;
  config.playerCount = 3;

  config.includePlayerId = {"A", "B", "C"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"W",
       {{0, 0, 0},
        {1, 0, 0},
        {2, 0, 0},
        {3, 0, 0},
        {4, 0, 0},
        {0, 1, 0},
        {4, 1, 0},
        {0, 2, 0},
        {4, 2, 0},
        {0, 3, 0},
        {4, 3, 0},
        {0, 4, 0},
        {1, 4, 0},
        {2, 4, 0},
        {3, 4, 0},
        {4, 4, 0}}},
      {"A",
       {{1, 1, 0, 2},
        {1, 2, 0, 3},
        {1, 3, 0, 1}}},
      {"B",
       {{2, 1, 0, 2},
        {2, 2, 0, 3},
        {2, 3, 0, 1}}},
      {"C",
       {{3, 1, 0, 2},
        {3, 2, 0, 3},
        {3, 3, 0, 1}}}};

  runEntityObserverRTSTest(config, expectedEntityObservervations);
}

TEST(EntityObserverTest, multiPlayer_Outline_Global) {
  EntityObserverConfig config = {5, 5};
  config.playerId = 0;
  config.playerCount = 3;

  config.includePlayerId = {"A", "B", "C"};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"W",
       {{0, 0, 0},
        {1, 0, 0},
        {2, 0, 0},
        {3, 0, 0},
        {4, 0, 0},
        {0, 1, 0},
        {4, 1, 0},
        {0, 2, 0},
        {4, 2, 0},
        {0, 3, 0},
        {4, 3, 0},
        {0, 4, 0},
        {1, 4, 0},
        {2, 4, 0},
        {3, 4, 0},
        {4, 4, 0}}},
      {"A",
       {{1, 1, 0, 1},
        {1, 2, 0, 2},
        {1, 3, 0, 3}}},
      {"B",
       {{2, 1, 0, 1},
        {2, 2, 0, 2},
        {2, 3, 0, 3}}},
      {"C",
       {{3, 1, 0, 1},
        {3, 2, 0, 2},
        {3, 3, 0, 3}}}};
}
}  // namespace griddly