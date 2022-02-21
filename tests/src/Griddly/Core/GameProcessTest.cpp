#include <algorithm>
#include <memory>

#include "Griddly/Core/TurnBasedGameProcess.cpp"
#include "Mocks/Griddly/Core/GDY/MockGDYFactory.hpp"
#include "Mocks/Griddly/Core/GDY/MockTerminationHandler.hpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.hpp"
#include "Mocks/Griddly/Core/LevelGenerators/MockLevelGenerator.hpp"
#include "Mocks/Griddly/Core/MockGrid.hpp"
#include "Mocks/Griddly/Core/Observers/MockObserver.hpp"
#include "Mocks/Griddly/Core/Players/MockPlayer.hpp"
#include "TestUtils/common.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#define _V(X) std::make_shared<int32_t>(X)

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::UnorderedElementsAreArray;

namespace griddly {

MATCHER_P(ObserverConfigEqMatcher, definition, "") {
  auto isEqual = definition.gridXOffset == arg.gridXOffset &&
                 definition.gridYOffset == arg.gridYOffset &&
                 definition.rotateWithAvatar == arg.rotateWithAvatar;
  return isEqual;
}

std::shared_ptr<MockPlayer> mockPlayer(std::string playerName, uint32_t playerId, std::shared_ptr<GameProcess> gameProcessPtr, std::shared_ptr<MockObject> mockPlayerAvatarPtr, std::shared_ptr<MockObserver<>> mockPlayerObserverPtr) {
  auto mockPlayerPtr = std::make_shared<MockPlayer>();
  EXPECT_CALL(*mockPlayerPtr, getName)
      .WillRepeatedly(Return(playerName));
  EXPECT_CALL(*mockPlayerPtr, getId)
      .WillRepeatedly(Return(playerId));

  auto playerObserverConfig = ObserverConfig{4, 8, 0, 0, false};

  if (mockPlayerAvatarPtr != nullptr) {
    EXPECT_CALL(*mockPlayerPtr, setAvatar(Eq(mockPlayerAvatarPtr)))
        .Times(AnyNumber());
  }

  if (mockPlayerObserverPtr != nullptr) {
    EXPECT_CALL(*mockPlayerPtr, getObserver())
        .WillRepeatedly(Return(mockPlayerObserverPtr));
  }

  return mockPlayerPtr;
}

TEST(GameProcessTest, init) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::VECTOR, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects)
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
//       .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr, mockPlayerObserverPtr);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initAlreadyInitialized) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::VECTOR, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects)
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

//   auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
//   EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
//       .Times(1);
  EXPECT_CALL(*mockObserverPtr, getObserverType())
      .WillRepeatedly(Return(ObserverType::VECTOR));

//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
//       .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr, mockPlayerObserverPtr);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_THROW(gameProcessPtr->init(), std::runtime_error);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initNoPlayerObserverDefinition) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::VECTOR, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .Times(1)
      .WillOnce(Return(mockObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects)
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

//   auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
//   EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
    //   .Times(1);
  EXPECT_CALL(*mockObserverPtr, getObserverType())
      .WillRepeatedly(Return(ObserverType::VECTOR));

//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
//       .WillOnce(Return(PlayerObserverDefinition{0, 0, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = std::make_shared<MockPlayer>();
  EXPECT_CALL(*mockPlayerPtr, getName)
      .WillRepeatedly(Return("Bob"));
  EXPECT_CALL(*mockPlayerPtr, getId)
      .WillRepeatedly(Return(1));
  EXPECT_CALL(*mockPlayerPtr, getObserver())
      .WillRepeatedly(Return(mockPlayerObserverPtr));

  auto defaultObserverConfig = ObserverConfig{};

  if (mockPlayerAvatarPtr != nullptr) {
    EXPECT_CALL(*mockPlayerPtr, setAvatar(Eq(mockPlayerAvatarPtr)))
        .Times(1);
  }

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initWrongNumberOfPlayers) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::VECTOR, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(10), Eq(0)))
      .Times(1)
      .WillOnce(Return(mockObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects)
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

//   auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
//   EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
//       .Times(1);

//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
//       .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(10));

  auto mockPlayerPtr = std::make_shared<MockPlayer>();
  EXPECT_CALL(*mockPlayerPtr, getObserver())
      .WillRepeatedly(Return(mockObserverPtr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  ASSERT_THROW(gameProcessPtr->init(), std::invalid_argument);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, reset) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::VECTOR, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillRepeatedly(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .Times(1)
      .WillOnce(Return(mockObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

//   auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
//   EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
//       .Times(1);
  EXPECT_CALL(*mockObserverPtr, getObserverType())
      .WillRepeatedly(Return(ObserverType::VECTOR));

//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
//       .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr, mockPlayerObserverPtr);
  EXPECT_CALL(*mockPlayerPtr, reset())
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillRepeatedly(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();
  gameProcessPtr->reset();

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  ASSERT_TRUE(gameProcessPtr->isInitialized());
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, resetNotInitialized) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::VECTOR, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ASSERT_THROW(gameProcessPtr->reset(), std::runtime_error);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

TEST(GameProcessTest, addPlayer) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockPlayerPtr1 = mockPlayer("Bob", 1, nullptr, nullptr, nullptr);
  auto mockPlayerPtr2 = mockPlayer("Alice", 2, nullptr, nullptr, nullptr);
  auto mockPlayerPtr3 = mockPlayer("Obama", 3, nullptr, nullptr, nullptr);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(3));

  gameProcessPtr->addPlayer(mockPlayerPtr1);
  gameProcessPtr->addPlayer(mockPlayerPtr2);
  gameProcessPtr->addPlayer(mockPlayerPtr3);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr2.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr3.get()));
}

TEST(GameProcessTest, addTooManyPlayers) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  auto mockPlayerPtr1 = mockPlayer("Bob", 1, nullptr, nullptr, nullptr);
  auto mockPlayerPtr2 = mockPlayer("Alice", 2, nullptr, nullptr, nullptr);
  auto mockPlayerPtr3 = std::make_shared<MockPlayer>();
  EXPECT_CALL(*mockPlayerPtr3, getName)
      .WillRepeatedly(Return("Obama"));
  EXPECT_CALL(*mockPlayerPtr3, getId)
      .WillRepeatedly(Return(3));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(2));

  gameProcessPtr->addPlayer(mockPlayerPtr1);
  gameProcessPtr->addPlayer(mockPlayerPtr2);

  ASSERT_THROW(gameProcessPtr->addPlayer(mockPlayerPtr3), std::invalid_argument);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr2.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr3.get()));
}

TEST(GameProcessTest, performActions) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  uint32_t playerId = 1;

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .WillRepeatedly(Return(std::make_shared<int32_t>(0)));
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .WillRepeatedly(ReturnRef(globalVariables));
  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects())
      .WillRepeatedly(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{}));
  EXPECT_CALL(*mockGridPtr, resetGlobalVariables(_))
      .Times(2);

  auto mockLevelGeneratorPtr = std::shared_ptr<MockLevelGenerator>(new MockLevelGenerator());
  auto mockTerminationHandlerPtr = std::shared_ptr<MockTerminationHandler>(new MockTerminationHandler(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(2);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator)
      .WillRepeatedly(Return(mockLevelGeneratorPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount)
      .WillRepeatedly(Return(1));
//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition)
//       .WillRepeatedly(Return(PlayerObserverDefinition{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler(Eq(mockGridPtr), _))
      .WillRepeatedly(Return(mockTerminationHandlerPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("NONE"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, nullptr, mockObserverPtr);

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();
  gameProcessPtr->reset();

  auto mockActionPtr = std::make_shared<MockAction>();

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockGridPtr, performActions(Eq(playerId), Eq(actionsList)))
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 14}}));

  EXPECT_CALL(*mockTerminationHandlerPtr, isTerminated())
      .WillOnce(Return(TerminationResult{false, {}}));

  EXPECT_CALL(*mockGridPtr, update())
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{}));

  auto result = gameProcessPtr->performActions(playerId, actionsList);

  ASSERT_FALSE(result.terminated);

  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(1), 14);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationHandlerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
}

TEST(GameProcessTest, performActionsMultiAgentRewards) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  uint32_t player1Id = 1;
  uint32_t player2Id = 2;
  uint32_t player3Id = 3;

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .WillRepeatedly(Return(std::make_shared<int32_t>(0)));
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .WillRepeatedly(ReturnRef(globalVariables));
  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects())
      .WillRepeatedly(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{}));
  EXPECT_CALL(*mockGridPtr, resetGlobalVariables(_))
      .Times(2);

  auto mockLevelGeneratorPtr = std::shared_ptr<MockLevelGenerator>(new MockLevelGenerator());
  auto mockTerminationHandlerPtr = std::shared_ptr<MockTerminationHandler>(new MockTerminationHandler(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(2);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator)
      .WillRepeatedly(Return(mockLevelGeneratorPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount)
      .WillRepeatedly(Return(3));
//   EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition)
//       .WillRepeatedly(Return(PlayerObserverDefinition{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler(Eq(mockGridPtr), _))
      .WillRepeatedly(Return(mockTerminationHandlerPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("NONE"), Eq(3), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  auto mockPlayerPtr1 = mockPlayer("Bob", 1, gameProcessPtr, nullptr, mockObserverPtr);
  auto mockPlayerPtr2 = mockPlayer("Spiff", 2, gameProcessPtr, nullptr, mockObserverPtr);
  auto mockPlayerPtr3 = mockPlayer("Hodor", 3, gameProcessPtr, nullptr, mockObserverPtr);

  gameProcessPtr->addPlayer(mockPlayerPtr1);
  gameProcessPtr->addPlayer(mockPlayerPtr2);
  gameProcessPtr->addPlayer(mockPlayerPtr3);

  gameProcessPtr->init();
  gameProcessPtr->reset();

  auto mockActionPtr = std::make_shared<MockAction>();

  auto actionsList = std::vector<std::shared_ptr<Action>>{mockActionPtr};

  EXPECT_CALL(*mockGridPtr, performActions(Eq(player1Id), Eq(actionsList)))
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 5}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(player2Id), Eq(actionsList)))
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{3, 10}, {2, -5}}));

  EXPECT_CALL(*mockGridPtr, performActions(Eq(player3Id), Eq(actionsList)))
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{3, 5}}));

  EXPECT_CALL(*mockTerminationHandlerPtr, isTerminated())
      .WillRepeatedly(Return(TerminationResult{false, {}}));

  EXPECT_CALL(*mockGridPtr, update())
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 5}}));

  auto result1 = gameProcessPtr->performActions(player1Id, actionsList);

  EXPECT_CALL(*mockGridPtr, update())
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{}));

  auto result2 = gameProcessPtr->performActions(player2Id, actionsList);

  EXPECT_CALL(*mockGridPtr, update())
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{}));

  auto result3 = gameProcessPtr->performActions(player3Id, actionsList);

  ASSERT_FALSE(result1.terminated);
  ASSERT_FALSE(result2.terminated);
  ASSERT_FALSE(result3.terminated);

  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(player1Id), 10);
  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(player2Id), -5);
  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(player3Id), 15);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationHandlerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr2.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr3.get()));
}

TEST(GameProcessTest, performActionsDelayedReward) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>> globalVariables{};

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .WillRepeatedly(Return(std::make_shared<int32_t>(0)));
  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .WillRepeatedly(ReturnRef(globalVariables));
  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects())
      .WillRepeatedly(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{}));
  EXPECT_CALL(*mockGridPtr, resetGlobalVariables(_))
      .Times(2);

  auto mockLevelGeneratorPtr = std::shared_ptr<MockLevelGenerator>(new MockLevelGenerator());
  auto mockTerminationHandlerPtr = std::shared_ptr<MockTerminationHandler>(new MockTerminationHandler(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr));

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(2);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator)
      .WillRepeatedly(Return(mockLevelGeneratorPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount)
      .WillRepeatedly(Return(1));
  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler(Eq(mockGridPtr), _))
      .WillRepeatedly(Return(mockTerminationHandlerPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(Return(std::unordered_map<std::string, GlobalVariableDefinition>{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("NONE"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, nullptr, mockObserverPtr);

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();
  gameProcessPtr->reset();

  auto mockActionPtr = std::make_shared<MockAction>();

  std::vector<std::shared_ptr<Action>> actionList{mockActionPtr};

  EXPECT_CALL(*mockGridPtr, performActions(Eq(1), Eq(actionList)))
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 14}, {2, 3}}));

  EXPECT_CALL(*mockTerminationHandlerPtr, isTerminated)
      .WillOnce(Return(TerminationResult{false, {}}));

  EXPECT_CALL(*mockGridPtr, update())
      .WillOnce(Return(std::unordered_map<uint32_t, int32_t>{{1, 5}, {5, 3}}));

  auto result = gameProcessPtr->performActions(1, actionList);

  ASSERT_FALSE(result.terminated);

  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(1), 19);
  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(2), 3);
  ASSERT_EQ(gameProcessPtr->getAccumulatedRewards(5), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockLevelGeneratorPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockTerminationHandlerPtr.get()));
}

TEST(GameProcessTest, getAvailableActionNames) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockObject1 = mockObject("object", 'o', 0, 0, {0, 1}, DiscreteOrientation(), {"move", "internal"});
  auto mockObject2 = mockObject("object", 'o', 1, 0, {4, 6}, DiscreteOrientation(), {"move", "fire"});
  auto mockObject3 = mockObject("object", 'o', 1, 0, {20, 13}, DiscreteOrientation(), {});

  auto objects = std::unordered_set<std::shared_ptr<Object>>{mockObject1, mockObject2, mockObject3};

  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  std::unordered_map<std::string, ActionInputsDefinition> mockActionInputsDefinitions = {
      {"move", {{}, false, false}},
      {"internal", {{}, false, true}},
      {"fire", {{}, false, false}}};

  EXPECT_CALL(*mockGDYFactoryPtr, getActionInputsDefinitions)
      .Times(1)
      .WillRepeatedly(Return(mockActionInputsDefinitions));

  EXPECT_CALL(*mockGridPtr, getObjects())
      .WillOnce(ReturnRef(objects));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  auto availableActionNames = gameProcessPtr->getAvailableActionNames(1);

  ASSERT_EQ(availableActionNames.size(), 1);
  ASSERT_THAT((availableActionNames[{4, 6}]), UnorderedElementsAreArray({"move", "fire"}));
}  // namespace griddly

TEST(GameProcessTest, getAvailableActionNames_empty) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  auto objects = std::unordered_set<std::shared_ptr<Object>>{};
  EXPECT_CALL(*mockGridPtr, getObjects())
      .WillOnce(ReturnRef(objects));

  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  std::unordered_map<std::string, ActionInputsDefinition> mockActionInputsDefinitions = {
      {"move", {{}, false, false}},
      {"internal", {{}, false, true}},
      {"fire", {{}, false, false}}};

  EXPECT_CALL(*mockGDYFactoryPtr, getActionInputsDefinitions)
      .Times(1)
      .WillRepeatedly(Return(mockActionInputsDefinitions));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  auto availableActionNames = gameProcessPtr->getAvailableActionNames(1);
  ASSERT_EQ(availableActionNames.size(), 0);
}

std::shared_ptr<MockAction> mockTestAction(std::string actionName, glm::ivec2 vectorToDest) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(Return(actionName));

  EXPECT_CALL(*mockActionPtr, getVectorToDest())
      .WillRepeatedly(Return(vectorToDest));

  return mockActionPtr;
}

MATCHER_P2(ActionAndVectorEqMatcher, actionName, vectorToDest, "") {
  auto isEqual = arg->getActionName() == actionName &&
                 arg->getVectorToDest() == vectorToDest;

  return isEqual;
}

TEST(GameProcessTest, getAvailableIdsForActionType) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  auto objectLocation = glm::ivec2{0, 1};
  auto mockObject1 = mockObject("object", 'o', 1, 0, objectLocation, DiscreteOrientation(), {"move", "attack"});

  EXPECT_CALL(*mockGridPtr, getObject(Eq(objectLocation)))
      .Times(2)
      .WillRepeatedly(Return(mockObject1));

  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();

  std::unordered_map<std::string, ActionInputsDefinition> mockActionInputsDefinitions = {
      {
          "move",
          {{{1, {{0, 1}, {0, 0}, "First Action"}},
            {2, {{0, 2}, {0, 0}, "Second Action"}},
            {3, {{0, 3}, {0, 0}, "Third Action"}}},
           false,
           false},
      },
      {"attack",
       {{{1, {{1, 0}, {0, 0}, "First Action"}},
         {2, {{2, 0}, {0, 0}, "Second Action"}},
         {3, {{3, 0}, {0, 0}, "Third Action"}},
         {4, {{4, 0}, {0, 0}, "Fourth Action"}},
         {5, {{5, 0}, {0, 0}, "Fifth Action"}},
         {6, {{6, 0}, {0, 0}, "Sixth Action"}}},
        false,
        false}}};

  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("move", glm::ivec2{0, 1}))).WillOnce(Return(true));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("move", glm::ivec2{0, 2}))).WillOnce(Return(false));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("move", glm::ivec2{0, 3}))).WillOnce(Return(true));

  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("attack", glm::ivec2{1, 0}))).WillOnce(Return(false));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("attack", glm::ivec2{2, 0}))).WillOnce(Return(true));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("attack", glm::ivec2{3, 0}))).WillOnce(Return(false));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("attack", glm::ivec2{4, 0}))).WillOnce(Return(true));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("attack", glm::ivec2{5, 0}))).WillOnce(Return(false));
  EXPECT_CALL(*mockObject1, isValidAction(ActionAndVectorEqMatcher("attack", glm::ivec2{6, 0}))).WillOnce(Return(true));

  EXPECT_CALL(*mockGDYFactoryPtr, getActionInputsDefinitions)
      .Times(2)
      .WillRepeatedly(Return(mockActionInputsDefinitions));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  auto availableMoveActionIds = gameProcessPtr->getAvailableActionIdsAtLocation(objectLocation, "move");
  auto availableAttackActionIds = gameProcessPtr->getAvailableActionIdsAtLocation(objectLocation, "attack");

  ASSERT_THAT(availableMoveActionIds, UnorderedElementsAreArray({1, 3}));
  ASSERT_THAT(availableAttackActionIds, UnorderedElementsAreArray({2, 4, 6}));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

TEST(GameProcessTest, getState) {
  auto mockGridPtr = std::make_shared<MockGrid>();

  auto globalVar = _V(5);
  auto playerVar = _V(6);

  auto mockObject1 = mockObject("object1", 'a', 0, 0, {0, 1}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param1", _V(20)}});
  auto mockObject2 = mockObject("object2", 'b', 1, 0, {4, 6}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param2", _V(5)}});
  auto mockObject3 = mockObject("object3", 'c', 1, 0, {20, 13}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param3", _V(12)}});

  auto objects = std::unordered_set<std::shared_ptr<Object>>{mockObject1, mockObject2, mockObject3};

  auto globalVariables = std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>{
      {"global_var", {{0, globalVar}}},
      {"player_var", {{1, playerVar}}}};

  EXPECT_CALL(*mockGridPtr, getObjects())
      .WillOnce(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .WillOnce(Return(_V(10)));

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .WillRepeatedly(ReturnRef(globalVariables));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, nullptr, mockGridPtr);

  auto state = gameProcessPtr->getState();

  ASSERT_EQ(state.gameTicks, 10);
  ASSERT_EQ(state.globalVariables.size(), 2);
  ASSERT_EQ(state.globalVariables["global_var"][0], *globalVar);
  ASSERT_EQ(state.globalVariables["player_var"][1], *playerVar);
  ASSERT_EQ(state.objectInfo.size(), 3);

  std::sort(state.objectInfo.begin(), state.objectInfo.end(), [](const ObjectInfo& a, const ObjectInfo& b) {
    return a.name < b.name;
  });

  ASSERT_EQ(state.objectInfo[0].name, "object1");
  ASSERT_EQ(state.objectInfo[0].playerId, 0);
  ASSERT_EQ(state.objectInfo[0].location, glm::ivec2(0, 1));
  ASSERT_EQ(state.objectInfo[0].variables.size(), 1);
  ASSERT_EQ(state.objectInfo[0].variables["test_param1"], 20);

  ASSERT_EQ(state.objectInfo[1].name, "object2");
  ASSERT_EQ(state.objectInfo[1].playerId, 1);
  ASSERT_EQ(state.objectInfo[1].location, glm::ivec2(4, 6));
  ASSERT_EQ(state.objectInfo[1].variables.size(), 1);
  ASSERT_EQ(state.objectInfo[1].variables["test_param2"], 5);

  ASSERT_EQ(state.objectInfo[2].name, "object3");
  ASSERT_EQ(state.objectInfo[2].playerId, 1);
  ASSERT_EQ(state.objectInfo[2].location, glm::ivec2(20, 13));
  ASSERT_EQ(state.objectInfo[2].variables.size(), 1);
  ASSERT_EQ(state.objectInfo[2].variables["test_param3"], 12);
}

TEST(GameProcessTest, clone) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();

  auto globalVar = _V(5);
  auto playerVar = _V(6);

  auto mockObject1 = mockObject("object1", 'a', 0, 0, {0, 1}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param1", _V(20)}});
  auto mockObject2 = mockObject("object2", 'b', 1, 0, {4, 6}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param2", _V(5)}});
  auto mockObject3 = mockObject("object3", 'c', 1, 0, {20, 13}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param3", _V(12)}});

  auto clonedMockObject1 = mockObject("object1", 'a', 0, 0, {0, 1}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param1", _V(20)}});
  auto clonedMockObject2 = mockObject("object2", 'b', 1, 0, {4, 6}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param2", _V(5)}});
  auto clonedMockObject3 = mockObject("object3", 'c', 1, 0, {20, 13}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param3", _V(12)}});

  auto mockPlayerDefaultObject = mockObject("_empty", ' ', 1, 0, {-1, -1}, DiscreteOrientation(), {}, {});

  auto objects = std::unordered_set<std::shared_ptr<Object>>{mockObject1, mockObject2, mockObject3};

  auto globalVariables = std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>{
      {"global_var", {{0, globalVar}}},
      {"player_var", {{1, playerVar}}}};

  EXPECT_CALL(*mockGridPtr, getWidth()).WillRepeatedly(Return(100));
  EXPECT_CALL(*mockGridPtr, getHeight()).WillRepeatedly(Return(100));

  EXPECT_CALL(*mockGridPtr, getObjects())
      .WillRepeatedly(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .WillRepeatedly(Return(_V(10)));

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .WillRepeatedly(ReturnRef(globalVariables));

  std::map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions = {
      {"object1", std::make_shared<ObjectDefinition>(ObjectDefinition{"object1", 'a'})},
      {"object2", std::make_shared<ObjectDefinition>(ObjectDefinition{"object2", 'b'})},
      {"object3", std::make_shared<ObjectDefinition>(ObjectDefinition{"object3", 'c'})},
  };

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions()).WillRepeatedly(ReturnRefOfCopy(mockObjectDefinitions));
  EXPECT_CALL(*mockObjectGeneratorPtr, cloneInstance(Eq(mockObject1), _)).WillRepeatedly(Return(clonedMockObject1));
  EXPECT_CALL(*mockObjectGeneratorPtr, cloneInstance(Eq(mockObject2), _)).WillRepeatedly(Return(clonedMockObject2));
  EXPECT_CALL(*mockObjectGeneratorPtr, cloneInstance(Eq(mockObject3), _)).WillRepeatedly(Return(clonedMockObject3));
  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_empty"), _, _)).WillRepeatedly(Return(mockPlayerDefaultObject));

  EXPECT_CALL(*mockGDYFactoryPtr, getObjectGenerator()).WillRepeatedly(Return(mockObjectGeneratorPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>(ObserverType::NONE, mockGDYFactoryPtr, mockGridPtr);

  for (int i = 0; i < 100; i++) {
    auto clonedPtr = gameProcessPtr->clone();
  }
}

}  // namespace griddly
