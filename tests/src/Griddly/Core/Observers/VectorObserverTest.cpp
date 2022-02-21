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

void runVectorObserverTest(VectorObserverConfig observerConfig,
                           Direction avatarDirection,
                           std::vector<uint32_t> expectedObservationShape,
                           std::vector<uint32_t> expectedObservationStride,
                           uint8_t* expectedData) {
  ObserverTestData testEnvironment = ObserverTestData(observerConfig, DiscreteOrientation(avatarDirection));

  std::shared_ptr<VectorObserver> vectorObserver = std::make_shared<VectorObserver>(VectorObserver(testEnvironment.mockGridPtr));

  vectorObserver->init(observerConfig);

  if (observerConfig.trackAvatar) {
    vectorObserver->setAvatar(testEnvironment.mockAvatarObjectPtr);
  }

  vectorObserver->reset();

  auto& updateObservation = vectorObserver->update();

  ASSERT_EQ(vectorObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(vectorObserver->getStrides(), expectedObservationStride);

  size_t dataLength = vectorObserver->getShape()[0] * vectorObserver->getShape()[1] * vectorObserver->getShape()[2];

  auto updateObservationPointer = std::vector<uint8_t>(&updateObservation, &updateObservation + dataLength);

  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData, dataLength));

  testEnvironment.verifyAndClearExpectations();
}

void runVectorObserverRTSTest(VectorObserverConfig observerConfig,
                              std::vector<uint32_t> expectedObservationShape,
                              std::vector<uint32_t> expectedObservationStride,
                              uint8_t* expectedData) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());

  ObserverRTSTestData testEnvironment = ObserverRTSTestData(observerConfig);

  std::shared_ptr<VectorObserver> vectorObserver = std::make_shared<VectorObserver>(VectorObserver(testEnvironment.mockGridPtr));

  vectorObserver->init(observerConfig);

  vectorObserver->reset();

  auto& updateObservation = vectorObserver->update();

  ASSERT_EQ(vectorObserver->getShape(), expectedObservationShape);
  ASSERT_EQ(vectorObserver->getStrides()[0], expectedObservationStride[0]);
  ASSERT_EQ(vectorObserver->getStrides()[1], expectedObservationStride[1]);

  size_t dataLength = vectorObserver->getShape()[0] * vectorObserver->getShape()[1] * vectorObserver->getShape()[2];

  auto updateObservationPointer = std::vector<uint8_t>(&updateObservation, &updateObservation + dataLength);

  ASSERT_THAT(updateObservationPointer, ElementsAreArray(expectedData, dataLength));

  testEnvironment.verifyAndClearExpectations();
}

TEST(VectorObserverTest, defaultObserverConfig) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, false};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver) {
  VectorObserverConfig config = {
      3,
      5,
      0,
      0,
      false, false};

  uint8_t expectedData[5][3][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 3, 5}, {1, 4, 12}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset) {
  VectorObserverConfig config = {
      3,
      5,
      0,
      1,
      false, false};

  uint8_t expectedData[5][3][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 3, 5}, {1, 4, 12}, expectedData[0][0]);
}

TEST(VectorObserverTest, defaultObserverConfig_trackAvatar) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_rotateWithAvatar_NONE) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_UP) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}
TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_RIGHT) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}
TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_DOWN) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}
TEST(VectorObserverTest, defaultObserverConfig_trackAvatar_LEFT) {
  VectorObserverConfig config = {
      5,
      5,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 5}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_NONE) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_UP) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_RIGHT) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_trackAvatar_DOWN) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}
TEST(VectorObserverTest, partialObserver_trackAvatar_LEFT) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      0,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_NONE) {
  VectorObserverConfig config = {
      5,
      3,
      1,
      1,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_UP) {
  VectorObserverConfig config = {
      5,
      3,
      1,
      1,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_RIGHT) {
  VectorObserverConfig config = {
      5,
      3,
      1,
      1,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_DOWN) {
  VectorObserverConfig config = {
      5,
      3,
      1,
      1,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}
TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_LEFT) {
  VectorObserverConfig config = {
      5,
      3,
      1,
      1,
      false, true};

  uint8_t expectedData[5][5][4] = {
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}},
      {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_NONE) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::NONE, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_UP) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::UP, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_RIGHT) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::RIGHT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_DOWN) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::DOWN, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, partialObserver_withOffset_trackAvatar_rotateWithAvatar_LEFT) {
  VectorObserverConfig config = {
      5,
      3,
      0,
      1,
      true, true};

  uint8_t expectedData[5][5][4] = {
      {{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {1, 0, 0, 0}},
      {{1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}}};

  runVectorObserverTest(config, Direction::LEFT, {4, 5, 3}, {1, 4, 20}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Player1) {
  VectorObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Player2) {
  VectorObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Player3) {
  VectorObserverConfig config = {5, 5, 0, 0};
  config.playerId = 3;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_Outline_Global) {
  VectorObserverConfig config = {5, 5, 0, 0};
  config.playerId = 0;
  config.playerCount = 3;

  config.includePlayerId = true;

  uint8_t expectedData[5][5][8] = {
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0}, {0, 1, 0, 0, 0, 0, 1, 0}, {0, 0, 1, 0, 0, 0, 1, 0}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 1}, {0, 1, 0, 0, 0, 0, 0, 1}, {0, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 0, 1, 1, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {8, 5, 5}, {1, 8, 8 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_PlusVariables_Player1) {
  VectorObserverConfig config = {5, 5, 0, 0};
  config.playerId = 1;
  config.playerCount = 3;

  config.includePlayerId = true;
  config.includeVariables = true;

  uint8_t expectedData[5][5][11] = {
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 1, 0, 0, 4, 0, 0}, {0, 0, 1, 0, 0, 1, 0, 0, 7, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0}, {0, 1, 0, 0, 0, 0, 1, 0, 0, 5, 0}, {0, 0, 1, 0, 0, 0, 1, 0, 0, 8, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 3}, {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 6}, {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 9}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {11, 5, 5}, {1, 11, 11 * 5}, expectedData[0][0]);
}

TEST(VectorObserverTest, multiPlayer_PlusVariables_Player2) {
  VectorObserverConfig config = {5, 5, 0, 0};
  config.playerId = 2;
  config.playerCount = 3;

  config.includePlayerId = true;
  config.includeVariables = true;

  uint8_t expectedData[5][5][11] = {
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0}, {0, 1, 0, 0, 0, 0, 1, 0, 4, 0, 0}, {0, 0, 1, 0, 0, 0, 1, 0, 7, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0}, {0, 1, 0, 0, 0, 1, 0, 0, 0, 5, 0}, {0, 0, 1, 0, 0, 1, 0, 0, 0, 8, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 3}, {0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 6}, {0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 9}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}},
      {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0}}};

  runVectorObserverRTSTest(config, {11, 5, 5}, {1, 11, 11 * 5}, expectedData[0][0]);
}

}  // namespace griddly