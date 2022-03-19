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
                           std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping,
                           EntityObservations& expectedEntityObservervations) {
  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection));

  std::shared_ptr<EntityObserver> entityObserver = std::make_shared<EntityObserver>(testEnvironment.mockGridPtr);

  entityObserver->init(observerConfig);

  if (observerConfig.trackAvatar) {
    entityObserver->setAvatar(testEnvironment.mockAvatarObjectPtr);
  }

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

// void runASCIIObserverRTSTest(ASCIIObserverConfig observerConfig,
//                              std::vector<uint32_t> expectedObservationShape,
//                              std::vector<uint32_t> expectedObservationStride,
//                              uint8_t* expectedData) {
//   auto mockGridPtr = std::make_shared<MockGrid>();

//   ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

//   std::shared_ptr<ASCIIObserver> asciiObserver = std::make_shared<ASCIIObserver>(testEnvironment.mockGridPtr);

//   asciiObserver->init(observerConfig);

//   asciiObserver->reset();

//   const auto& updateObservation = asciiObserver->update();

//   ASSERT_EQ(asciiObserver->getShape(), expectedObservationShape);
//   ASSERT_EQ(asciiObserver->getStrides()[0], expectedObservationStride[0]);
//   ASSERT_EQ(asciiObserver->getStrides()[1], expectedObservationStride[1]);

//   size_t dataLength = asciiObserver->getShape()[0] * asciiObserver->getShape()[1] * asciiObserver->getShape()[2];

//   auto updateObservationPointer = std::vector<uint8_t>(&updateObservation, &updateObservation + dataLength);

//   ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData, dataLength));

//   testEnvironment.verifyAndClearExpectations();
// }

TEST(EntityObserverTest, defaultObserverConfig) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      false, false};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;
  // "x", "y", "z", "ox", "oy", "player_id"
  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 3, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 3, 0, 0, 0, 1},
        {3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false, false};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      -1,
      false,
      false};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;
  // "x", "y", "z", "ox", "oy", "player_id"
  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 2, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1},
        {3, 0, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 3, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 3, 0, 0, 0, 1},
        {3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 3, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 3, 0, 0, 0, 1},
        {3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_UP) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, -1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 3, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 3, 0, 0, 0, 1},
        {3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 1, 0, 0, 0, 1},    // 3,1
        {1, 3, 0, 0, 0, 1},    // 1,3
        {2, 3, 0, 0, 0, 1}}},  // 2,3
      {"mo3",
       {{3, 3, 0, 0, 0, 1},     // 3, 3
        {2, 1, 0, 0, 0, 1},     // 2, 1
        {1, 1, 0, 0, 0, 1}}}};  // 1, 1

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {3, 3, 0, 0, 0, 1},
        {3, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1},
        {1, 3, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      5,
      0,
      0,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, -1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1},
        {0, 3, -1, 0, 0, 1},
        {4, 3, -1, 0, 0, 1},
        {0, 4, -1, 0, 0, 1},
        {1, 4, -1, 0, 0, 1},
        {2, 4, -1, 0, 0, 1},
        {3, 4, -1, 0, 0, 1},
        {4, 4, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 3, 0, 0, 0, 1},    // 1,3
        {3, 1, 0, 0, 0, 1},    // 3,1
        {2, 1, 0, 0, 0, 1}}},  // 2,1
      {"mo3",
       {{1, 1, 0, 0, 0, 1},     // 1,1
        {2, 3, 0, 0, 0, 1},     // 2,3
        {3, 3, 0, 0, 0, 1}}}};  // 3,3

  runEntityObserverTest(config, Direction::LEFT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 2, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1},
        {3, 0, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_UP) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, -1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 2, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1},
        {3, 0, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 2, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1},
        {3, 0, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, 0, 1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 2, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1},
        {3, 0, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_trackAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      0,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 1, 0, -1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 2, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{1, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1},
        {3, 0, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::LEFT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, -1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      false,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, -1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::LEFT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::NONE, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, -1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 2, 0, 0, 0, 1},
        {3, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::UP, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 1, 0, 0, 0, 1}}},
      {"mo3",
       {{2, 1, 0, 0, 0, 1},
        {1, 1, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::RIGHT, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, 0, 1, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{1, 1, 0, 0, 0, 1},
        {3, 2, 0, 0, 0, 1}}},
      {"mo3",
       {{3, 1, 0, 0, 0, 1},
        {1, 2, 0, 0, 0, 1}}}};

  runEntityObserverTest(config, Direction::DOWN, expectedEntityVariableMapping, expectedEntityObservervations);
}

TEST(EntityObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  EntityObserverConfig config = {
      5,
      3,
      0,
      1,
      true,
      true};

  std::unordered_map<std::string, std::vector<std::string>> expectedEntityVariableMapping = {};

  EntityObservations expectedEntityObservervations;

  expectedEntityObservervations.observations = {
      {"avatar",
       {{2, 2, 0, -1, 0, 1}}},
      {"mo1",
       {{0, 0, -1, 0, 0, 1},
        {1, 0, -1, 0, 0, 1},
        {2, 0, -1, 0, 0, 1},
        {3, 0, -1, 0, 0, 1},
        {4, 0, -1, 0, 0, 1},
        {0, 1, -1, 0, 0, 1},
        {4, 1, -1, 0, 0, 1},
        {0, 2, -1, 0, 0, 1},
        {4, 2, -1, 0, 0, 1}}},
      {"mo2",
       {{3, 1, 0, 0, 0, 1},    // 3,1
        {2, 1, 0, 0, 0, 1}}},  // 2,1
      {"mo3",
       {{1, 1, 0, 0, 0, 1}}}};  // 3,3

  runEntityObserverTest(config, Direction::LEFT, expectedEntityVariableMapping, expectedEntityObservervations);
}

// TEST(ASCIIObserverTest, multiPlayer_Outline_Player1) {
//   ASCIIObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 1;
//   config.playerCount = 3;

//   config.includePlayerId = true;

//   uint8_t expectedData[5][5][4] = {
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '1', ' ', ' '}, {'B', '1', ' ', ' '}, {'C', '1', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '2', ' ', ' '}, {'B', '2', ' ', ' '}, {'C', '2', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '3', ' ', ' '}, {'B', '3', ' ', ' '}, {'C', '3', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}}};

//   runASCIIObserverRTSTest(config, {4, 5, 5}, {1, 4, 4 * 5}, expectedData[0][0]);
// }

// TEST(ASCIIObserverTest, multiPlayer_Outline_Player2) {
//   ASCIIObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 2;
//   config.playerCount = 3;

//   config.includePlayerId = true;

//   uint8_t expectedData[5][5][4] = {
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '2', ' ', ' '}, {'B', '2', ' ', ' '}, {'C', '2', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '1', ' ', ' '}, {'B', '1', ' ', ' '}, {'C', '1', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '3', ' ', ' '}, {'B', '3', ' ', ' '}, {'C', '3', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}}};

//   runASCIIObserverRTSTest(config, {4, 5, 5}, {1, 4, 4 * 5}, expectedData[0][0]);
// }

// TEST(ASCIIObserverTest, multiPlayer_Outline_Player3) {
//   ASCIIObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 3;
//   config.playerCount = 3;

//   config.includePlayerId = true;

//   uint8_t expectedData[5][5][4] = {
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '2', ' ', ' '}, {'B', '2', ' ', ' '}, {'C', '2', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '3', ' ', ' '}, {'B', '3', ' ', ' '}, {'C', '3', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '1', ' ', ' '}, {'B', '1', ' ', ' '}, {'C', '1', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}}};

//   runASCIIObserverRTSTest(config, {4, 5, 5}, {1, 4, 4 * 5}, expectedData[0][0]);
// }

// TEST(ASCIIObserverTest, multiPlayer_Outline_Global) {
//   ASCIIObserverConfig config = {5, 5, 0, 0};
//   config.playerId = 0;
//   config.playerCount = 3;

//   config.includePlayerId = true;

//   uint8_t expectedData[5][5][4] = {
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '1', ' ', ' '}, {'B', '1', ' ', ' '}, {'C', '1', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '2', ' ', ' '}, {'B', '2', ' ', ' '}, {'C', '2', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'A', '3', ' ', ' '}, {'B', '3', ' ', ' '}, {'C', '3', ' ', ' '}, {'W', ' ', ' ', ' '}},
//       {{'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}, {'W', ' ', ' ', ' '}}};

//   runASCIIObserverRTSTest(config, {4, 5, 5}, {1, 4, 4 * 5}, expectedData[0][0]);
// }
}  // namespace griddly