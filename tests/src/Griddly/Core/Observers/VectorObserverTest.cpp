#include "Griddly/Core/GDY/Objects/Object.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/Observers/VectorObserver.hpp"
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

void runVectorObserverTest(ObserverConfig observerConfig,
                           Direction avatarDirection,
                           std::vector<uint32_t> expectedObservationShape,
                           std::vector<uint32_t> expectedObservationStride,
                           uint8_t* expectedData,
                           bool trackAvatar) {
  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection), trackAvatar);

  std::shared_ptr<VectorObserver> vectorObserver = std::shared_ptr<VectorObserver>(new VectorObserver(testEnvironment.mockGridPtr));

  vectorObserver->init(observerConfig);

  if (trackAvatar) {
    vectorObserver->setAvatar(testEnvironment.mockAvatarObjectPtr);
  }
  auto resetObservation = vectorObserver->reset();

  ASSERT_EQ(vectorObserver->getTileSize(), glm::ivec2(1, 1));
  ASSERT_EQ(vectorObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(vectorObserver->getStrides(), expectedObservationStride);

  auto updateObservation = vectorObserver->update();

  size_t dataLength = vectorObserver->getShape()[0] * vectorObserver->getShape()[1] * vectorObserver->getShape()[2];

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation, resetObservation + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation, updateObservation + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedData, dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData, dataLength));

  testEnvironment.verifyAndClearExpectations();
}

void runVectorObserverRTSTest(ObserverConfig observerConfig,
                              std::vector<uint32_t> expectedObservationShape,
                              std::vector<uint32_t> expectedObservationStride,
                              uint8_t* expectedData) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<VectorObserver> vectorObserver = std::shared_ptr<VectorObserver>(new VectorObserver(testEnvironment.mockGridPtr));

  vectorObserver->init(observerConfig);

  auto resetObservation = vectorObserver->reset();

  ASSERT_EQ(vectorObserver->getTileSize(), glm::ivec2(1, 1));
  ASSERT_EQ(vectorObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(vectorObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(vectorObserver->getStrides()[1], expectedObservationStride[1]);

  auto updateObservation = vectorObserver->update();

  size_t dataLength = vectorObserver->getShape()[0] * vectorObserver->getShape()[1] * vectorObserver->getShape()[2];

  auto resetObservationPointer = std::vector<uint8_t>(resetObservation, resetObservation + dataLength);
  auto updateObservationPointer = std::vector<uint8_t>(updateObservation, updateObservation + dataLength);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(expectedData, dataLength));
  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData, dataLength));

  testEnvironment.verifyAndClearExpectations();
}

TEST(VectorObserverTest, defaultObserverConfig) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}

TEST(VectorObserverTest, partialObserver) {
  ObserverConfig config = {
      3,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][3][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 3, 5}, {1, 4, 12}, expectedData[0][0], false);
}

TEST(VectorObserverTest, partialObserver_withOffset) {
  ObserverConfig config = {
      3,
      5,
      0,
      1,
      false};

  uint8_t expectedData[5][3][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 3, 5}, {1, 4, 12}, expectedData[0][0], false);
}

TEST(VectorObserverTest, defaultObserverConfig_trackAvatar) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}

TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}

TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}
TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}
TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}
TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      5,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 5}, {1, 4, 20}, expectedData[0][0], false);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}
TEST(VectorObserverTest, partialObserver_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      0,
      false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      1,
      1,
      false};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      1,
      1,
      false};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      1,
      1,
      false};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      1,
      1,
      false};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}
TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      1,
      1,
      false};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  ObserverConfig config = {
      5,
      3,
      0,
      1,
      true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0], true);
}

TEST(VectorObserverTest, multiPlayer_Outline_Player1) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 1, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Player2) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Player3) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 0, 1}, {0, 0, 0, 1, 0, 0, 0, 1}, {0, 0, 1, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Global) {
  ObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 0, 1, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {1, 0, 0, 0, 1, 0, 0, 0}},
      {{1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}, {1, 0, 0, 0, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

}  // namespace griddly