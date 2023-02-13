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
using ::testing::ReturnRefOfCopy;
using ::testing::UnorderedElementsAreArray;

namespace griddly {

MATCHER_P(ObserverConfigEqMatcher, definition, "") {
  auto isEqual = definition.gridXOffset == arg.gridXOffset &&
                 definition.gridYOffset == arg.gridYOffset &&
                 definition.rotateWithAvatar == arg.rotateWithAvatar;
  return isEqual;
}

MATCHER_P(GameObjectDataMatcher, objectData, "") {
  auto isEqual = objectData.name == arg.name &&
                 objectData.variables == arg.variables;
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
    EXPECT_CALL(*mockPlayerPtr, reset(Eq(mockPlayerAvatarPtr)))
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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("VECTOR", mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(1), Eq(1)))
      .WillOnce(Return(mockPlayerObserverPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr, mockPlayerObserverPtr);

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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("VECTOR", mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(1), Eq(1)))
      .WillOnce(Return(mockPlayerObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockObserverPtr, getObserverType())
      .WillRepeatedly(Return(ObserverType::VECTOR));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr, mockPlayerObserverPtr);

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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("VECTOR", mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .Times(1)
      .WillOnce(Return(mockObserverPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(1), Eq(1)))
      .WillOnce(Return(mockPlayerObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

  EXPECT_CALL(*mockObserverPtr, getObserverType())
      .WillRepeatedly(Return(ObserverType::VECTOR));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = std::make_shared<MockPlayer>();
  EXPECT_CALL(*mockPlayerPtr, getName)
      .WillRepeatedly(Return("Bob"));
  EXPECT_CALL(*mockPlayerPtr, getId)
      .WillRepeatedly(Return(1));
  EXPECT_CALL(*mockPlayerPtr, getObserver())
      .WillRepeatedly(Return(mockPlayerObserverPtr));

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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("VECTOR", mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillOnce(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(1);

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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("VECTOR", mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));
  auto mockPlayerObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator(Eq(0)))
      .WillRepeatedly(Return(mockLevelGeneratorPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("VECTOR"), Eq(1), Eq(0)))
      .Times(1)
      .WillOnce(Return(mockObserverPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(1), Eq(1)))
      .WillOnce(Return(mockPlayerObserverPtr));

  auto mockPlayerAvatarPtr = std::make_shared<MockObject>();
  EXPECT_CALL(*mockObserverPtr, getObserverType())
      .WillRepeatedly(Return(ObserverType::VECTOR));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr, mockPlayerObserverPtr);
  EXPECT_CALL(*mockPlayerPtr, reset(Eq(mockPlayerAvatarPtr)))
      .Times(1);

  EXPECT_CALL(*mockObserverPtr, reset(Eq(nullptr)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getPlayerAvatarObjects())
      .WillRepeatedly(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("VECTOR", mockGDYFactoryPtr, mockGridPtr);
  auto mockLevelGeneratorPtr = std::make_shared<MockLevelGenerator>();

  ASSERT_THROW(gameProcessPtr->reset(), std::runtime_error);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

TEST(GameProcessTest, addPlayer) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGDYFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockLevelGenerator = std::make_shared<MockLevelGenerator>();
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);
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
  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);
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

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

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
      .WillRepeatedly(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("NONE"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(1), Eq(1)))
      .WillOnce(Return(mockObserverPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

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

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

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
      .WillRepeatedly(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("NONE"), Eq(3), Eq(0)))
      .WillOnce(Return(mockObserverPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(3), Eq(1)))
      .WillOnce(Return(mockObserverPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(3), Eq(2)))
      .WillOnce(Return(mockObserverPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(3), Eq(3)))
      .WillOnce(Return(mockObserverPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

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

  ObserverConfig mockObserverConfig{};

  auto mockObserverPtr = std::shared_ptr<MockObserver<>>(new MockObserver<>(mockGridPtr, mockObserverConfig));

  EXPECT_CALL(*mockLevelGeneratorPtr, reset(Eq(mockGridPtr)))
      .Times(2);

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator)
      .WillRepeatedly(Return(mockLevelGeneratorPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount)
      .WillRepeatedly(Return(1));
  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler(Eq(mockGridPtr), _))
      .WillRepeatedly(Return(mockTerminationHandlerPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(ReturnRefOfCopy(std::map<std::string, GlobalVariableDefinition>{}));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("NONE"), Eq(1), Eq(0)))
      .WillOnce(Return(mockObserverPtr));
  EXPECT_CALL(*mockGDYFactoryPtr, createObserver(Eq(mockGridPtr), Eq("testObserver"), Eq(1), Eq(1)))
      .WillOnce(Return(mockObserverPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

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

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

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

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

  auto availableActionNames = gameProcessPtr->getAvailableActionNames(1);
  ASSERT_EQ(availableActionNames.size(), 0);
}

std::shared_ptr<MockAction> mockTestAction(std::string actionName, glm::ivec2 vectorToDest) {
  auto mockActionPtr = std::make_shared<MockAction>();

  EXPECT_CALL(*mockActionPtr, getActionName())
      .WillRepeatedly(ReturnRefOfCopy(actionName));

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

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

  auto availableMoveActionIds = gameProcessPtr->getAvailableActionIdsAtLocation(objectLocation, "move");
  auto availableAttackActionIds = gameProcessPtr->getAvailableActionIdsAtLocation(objectLocation, "attack");

  ASSERT_THAT(availableMoveActionIds, UnorderedElementsAreArray({1, 3}));
  ASSERT_THAT(availableAttackActionIds, UnorderedElementsAreArray({2, 4, 6}));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObject1.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

TEST(GameProcessTest, getGameState) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGdyFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();

  auto globalVar = _V(5);
  auto playerVar = _V(6);

  auto mockObject1 = mockObject("object1", 'a', 0, 0, {0, 1}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param1", _V(20)}});
  auto mockObject2 = mockObject("object2", 'b', 1, 0, {4, 6}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param2", _V(5)}});
  auto mockObject3 = mockObject("object3", 'c', 1, 0, {20, 13}, DiscreteOrientation(), {}, {{"global_var", globalVar}, {"test_param3", _V(12)}});
  auto mockGlobalDefaultEmptyObject = mockObject("_empty", '?', 0, 0, {-1, -1});
  auto mockGlobalDefaultBoundaryObject = mockObject("_boundary", '?', 0, 0, {-1, -1});
  auto mockPlayerDefaultEmptyObject = mockObject("_empty", '?', 1, 0, {-1, -1});
  auto mockPlayerDefaultBoundaryObject = mockObject("_boundary", '?', 1, 0, {-1, -1});

  auto objects = std::unordered_set<std::shared_ptr<Object>>{mockObject1, mockObject2, mockObject3};

  auto globalVariables = std::map<std::string, std::unordered_map<uint32_t, std::shared_ptr<int32_t>>>{
      {"_steps", {{0, _V(0)}}},
      {"global_var", {{0, globalVar}}},
      {"player_var", {{0, _V(0)}, {1, playerVar}}}};

  EXPECT_CALL(*mockGridPtr, getObjects())
      .WillOnce(ReturnRef(objects));

  EXPECT_CALL(*mockGridPtr, getTickCount())
      .WillOnce(Return(_V(10)));

  EXPECT_CALL(*mockGridPtr, getGlobalVariables())
      .WillRepeatedly(ReturnRef(globalVariables));

  EXPECT_CALL(*mockGridPtr, getPlayerDefaultEmptyObject(Eq(0))).WillOnce(Return(mockGlobalDefaultEmptyObject));
  EXPECT_CALL(*mockGridPtr, getPlayerDefaultBoundaryObject(Eq(0))).WillOnce(Return(mockGlobalDefaultBoundaryObject));

  EXPECT_CALL(*mockGridPtr, getPlayerDefaultEmptyObject(Eq(1))).WillOnce(Return(mockPlayerDefaultEmptyObject));
  EXPECT_CALL(*mockGridPtr, getPlayerDefaultBoundaryObject(Eq(1))).WillOnce(Return(mockPlayerDefaultBoundaryObject));

  EXPECT_CALL(*mockGridPtr, getHeight).WillRepeatedly(Return(10));
  EXPECT_CALL(*mockGridPtr, getWidth).WillRepeatedly(Return(9));

  EXPECT_CALL(*mockGridPtr, getPlayerCount).WillRepeatedly(Return(1));

  EXPECT_CALL(*mockGdyFactoryPtr, getObjectGenerator).WillRepeatedly(Return(mockObjectGeneratorPtr));

  GameStateMapping stateMapping;
  stateMapping.globalVariableNameToIdx = {
      {"_steps", 0},
      {"global_var", 1},
      {"player_var", 2}};
  stateMapping.addObject("object1");
  stateMapping.addObject("object2");
  stateMapping.addObject("object3");
  stateMapping.addObject("_empty");
  stateMapping.addObject("_boundary");
  stateMapping.addObjectVariable("object1", "test_param1");
  stateMapping.addObjectVariable("object2", "test_param2");
  stateMapping.addObjectVariable("object3", "test_param3");

  EXPECT_CALL(*mockObjectGeneratorPtr, getStateMapping)
      .WillRepeatedly(ReturnRef(stateMapping));
  GameObjectData object1Data{0, "object1", {0, 1, 0, 0, 0, 0, 20}};
  GameObjectData object2Data{0, "object2", {4, 6, 0, 0, 1, 0, 5}};
  GameObjectData object3Data{0, "object3", {20, 13, 0, 0, 1, 0, 12}};
  GameObjectData emptyObject{0, "_empty", {-1, -1, 0, 0, 1, 0}};
  GameObjectData boundaryObject{0, "_boundary", {-1, -1, 0, 0, 1, 0}};
  GameObjectData player1EmptyObject{0, "_empty", {-1, -1, 0, 0, 1, 0}};
  GameObjectData player1BoundaryObject{0, "_boundary", {-1, -1, 0, 0, 1, 0}};
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockObject1))).WillOnce(Return(object1Data));
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockObject2))).WillOnce(Return(object2Data));
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockObject3))).WillOnce(Return(object3Data));
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockGlobalDefaultEmptyObject))).WillOnce(Return(emptyObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockGlobalDefaultBoundaryObject))).WillOnce(Return(boundaryObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockPlayerDefaultEmptyObject))).WillOnce(Return(player1EmptyObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, toObjectData(Eq(mockPlayerDefaultBoundaryObject))).WillOnce(Return(player1BoundaryObject));

  auto mockDelayedAction1Ptr = mockAction("action1", 0, mockObject1, {3, 4}, {0, 1});
  auto mockDelayedAction2Ptr = mockAction("action2", 1, mockObject2, {4, 5}, {1, 0});

  auto delayedActionQueueItem1Ptr = std::make_shared<DelayedActionQueueItem>(1, 321, mockDelayedAction1Ptr);
  auto delayedActionQueueItem2Ptr = std::make_shared<DelayedActionQueueItem>(1, 123, mockDelayedAction2Ptr);

  DelayedActionQueue delayedActionQueue;
  delayedActionQueue.push(delayedActionQueueItem1Ptr);
  delayedActionQueue.push(delayedActionQueueItem2Ptr);

  EXPECT_CALL(*mockGridPtr, getDelayedActions)
      .WillOnce(ReturnRef(delayedActionQueue));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGdyFactoryPtr, mockGridPtr);
  auto state = gameProcessPtr->getGameState();

  ASSERT_EQ(state.tickCount, 10);
  ASSERT_EQ(state.grid.height, 10);
  ASSERT_EQ(state.grid.width, 9);
  ASSERT_EQ(state.playerCount, 1);
  ASSERT_EQ(state.globalData.size(), 3);
  ASSERT_EQ(state.globalData[stateMapping.globalVariableNameToIdx.at("_steps")][0], 0);
  ASSERT_EQ(state.globalData[stateMapping.globalVariableNameToIdx.at("global_var")][0], *globalVar);
  ASSERT_EQ(state.globalData[stateMapping.globalVariableNameToIdx.at("player_var")][0], 0);
  ASSERT_EQ(state.globalData[stateMapping.globalVariableNameToIdx.at("player_var")][1], *playerVar);

  ASSERT_EQ(state.objectData.size(), 7);

  uint32_t object1Idx = 0;
  uint32_t object2Idx = 0;
  uint32_t object3Idx = 0;
  for (uint32_t i = 0; i < 3; i++) {
    if (state.objectData[i].name == "object1") {
      object1Idx = i;
    } else if (state.objectData[i].name == "object2") {
      object2Idx = i;
    } else if (state.objectData[i].name == "object3") {
      object3Idx = i;
    }
  }

  ASSERT_EQ(state.objectData.size(), 7);

  const auto& object1VariableIndexes = state.objectData[object1Idx].getVariableIndexes(stateMapping);
  ASSERT_EQ(state.objectData[object1Idx].name, "object1");
  ASSERT_EQ(state.objectData[object1Idx].variables.size(), 7);
  ASSERT_EQ(state.objectData[object1Idx].getVariableValue(object1VariableIndexes, "test_param1"), 20);

  const auto& object2VariableIndexes = state.objectData[object2Idx].getVariableIndexes(stateMapping);
  ASSERT_EQ(state.objectData[object2Idx].name, "object2");
  ASSERT_EQ(state.objectData[object2Idx].variables.size(), 7);
  ASSERT_EQ(state.objectData[object2Idx].getVariableValue(object2VariableIndexes, "test_param2"), 5);

  const auto& object3VariableIndexes = state.objectData[object3Idx].getVariableIndexes(stateMapping);
  ASSERT_EQ(state.objectData[object3Idx].name, "object3");
  ASSERT_EQ(state.objectData[object3Idx].variables.size(), 7);
  ASSERT_EQ(state.objectData[object3Idx].getVariableValue(object3VariableIndexes, "test_param3"), 12);

  ASSERT_EQ(state.delayedActionData.size(), 2);
  ASSERT_EQ(state.delayedActionData.top().actionName, "action2");
  ASSERT_EQ(state.delayedActionData.top().orientationVector, glm::ivec2(1, 0));
  ASSERT_EQ(state.delayedActionData.top().originatingPlayerId, 1);
  ASSERT_EQ(state.delayedActionData.top().priority, 113);
  ASSERT_EQ(state.delayedActionData.top().sourceObjectIdx, object2Idx);
  ASSERT_EQ(state.delayedActionData.top().vectorToDest, glm::ivec2(4, 5));
  state.delayedActionData.pop();
  ASSERT_EQ(state.delayedActionData.top().actionName, "action1");
  ASSERT_EQ(state.delayedActionData.top().orientationVector, glm::ivec2(0, 1));
  ASSERT_EQ(state.delayedActionData.top().originatingPlayerId, 0);
  ASSERT_EQ(state.delayedActionData.top().priority, 311);
  ASSERT_EQ(state.delayedActionData.top().sourceObjectIdx, object1Idx);
  ASSERT_EQ(state.delayedActionData.top().vectorToDest, glm::ivec2(3, 4));
  state.delayedActionData.pop();
}

TEST(GameProcessTest, fromGameState) {
  auto mockGridPtr = std::make_shared<MockGrid>();
  auto mockGdyFactoryPtr = std::make_shared<MockGDYFactory>();
  auto mockObjectGeneratorPtr = std::make_shared<MockObjectGenerator>();

  auto globalVar = _V(5);
  auto playerVar = _V(6);

  GameStateMapping stateMapping;
  stateMapping.globalVariableNameToIdx = {
      {"_steps", 0},
      {"global_var", 1},
      {"player_var", 2}};
  stateMapping.addObject("object1");
  stateMapping.addObject("object2");
  stateMapping.addObject("object3");
  stateMapping.addObject("_empty");
  stateMapping.addObject("_boundary");
  stateMapping.addObjectVariable("object1", "test_param1");
  stateMapping.addObjectVariable("object2", "test_param2");
  stateMapping.addObjectVariable("object3", "test_param3");

  EXPECT_CALL(*mockGdyFactoryPtr, getObjectGenerator).WillRepeatedly(Return(mockObjectGeneratorPtr));

  EXPECT_CALL(*mockObjectGeneratorPtr, getStateMapping)
      .WillRepeatedly(ReturnRef(stateMapping));

  std::map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions = {
      {"object1", std::make_shared<ObjectDefinition>(ObjectDefinition{"object1", 'a'})},
      {"object2", std::make_shared<ObjectDefinition>(ObjectDefinition{"object2", 'b'})},
      {"object3", std::make_shared<ObjectDefinition>(ObjectDefinition{"object3", 'c'})},
  };

  auto loadedMockObject1 = mockObject("object1", 'a');
  auto loadedMockObject2 = mockObject("object2", 'b');
  auto loadedMockObject3 = mockObject("object3", 'c');
  auto mockPlayerDefaultEmptyObject = mockObject("_empty", ' ', 1, 0, {-1, -1}, DiscreteOrientation(), {}, {});
  auto mockPlayerDefaultBoundaryObject = mockObject("_boundary", ' ', 1, 0, {-1, -1}, DiscreteOrientation(), {}, {});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions()).WillRepeatedly(ReturnRefOfCopy(mockObjectDefinitions));

  GameState state = {
      0, 1, 10};
  state.grid = {10, 9};
  state.globalData = {{0}, {*globalVar}, {0, *playerVar}};
  state.objectData = {
      {0, "object1", {0, 1, 0, 0, 0, 0, 20}},
      {0, "object2", {4, 6, 0, 0, 1, 0, 5}},
      {0, "object3", {20, 13, 0, 0, 1, 0, 12}},
      {0, "_empty", {-1, -1, 0, 0, 0, 0}},
      {0, "_boundary", {-1, -1, 0, 0, 0, 0}},
      {0, "_empty", {-1, -1, 0, 0, 1, 0}},
      {0, "_boundary", {-1, -1, 0, 0, 1, 0}}};
  state.delayedActionData.push({100, 1, 0, "action1", {0, 1}, {0, 1}, 1});
  state.delayedActionData.push({100, 1, 0, "action2", {0, 1}, {0, 1}, 1});

  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[0]), _)).WillOnce(Return(loadedMockObject1));
  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[1]), _)).WillOnce(Return(loadedMockObject2));
  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[2]), _)).WillOnce(Return(loadedMockObject3));
  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[3]), _)).WillOnce(Return(mockPlayerDefaultEmptyObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[4]), _)).WillOnce(Return(mockPlayerDefaultBoundaryObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[5]), _)).WillOnce(Return(mockPlayerDefaultEmptyObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, fromObjectData(GameObjectDataMatcher(state.objectData[6]), _)).WillOnce(Return(mockPlayerDefaultBoundaryObject));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGdyFactoryPtr, mockGridPtr);

  auto newGameProcessPtr = gameProcessPtr->fromGameState(state);

  auto loadedGrid = newGameProcessPtr->getGrid();

  ASSERT_EQ(*loadedGrid->getTickCount(), 10);
  ASSERT_EQ(loadedGrid->getWidth(), 10);
  ASSERT_EQ(loadedGrid->getHeight(), 9);
  ASSERT_EQ(loadedGrid->getPlayerCount(), 1);

  auto loadedGlobalVariables = loadedGrid->getGlobalVariables();
  ASSERT_EQ(loadedGlobalVariables.size(), 3);
  ASSERT_EQ(*loadedGlobalVariables.at("_steps")[0], 10);
  ASSERT_EQ(*loadedGlobalVariables.at("global_var")[0], *globalVar);
  ASSERT_EQ(*loadedGlobalVariables.at("player_var")[0], 0);
  ASSERT_EQ(*loadedGlobalVariables.at("player_var")[1], *playerVar);

  //   ASSERT_EQ(state.objectData.size(), 3);

  //   uint32_t object1Idx = 0;
  //   uint32_t object2Idx = 0;
  //   uint32_t object3Idx = 0;
  //   for (uint32_t i = 0; i < 3; i++) {
  //     if (state.objectData[i].name == "object1") {
  //       object1Idx = i;
  //     } else if (state.objectData[i].name == "object2") {
  //       object2Idx = i;
  //     } else if (state.objectData[i].name == "object3") {
  //       object3Idx = i;
  //     }
  //   }

  //   std::sort(state.objectData.begin(), state.objectData.end(), [](const GameObjectData& a, const GameObjectData& b) {
  //     return a.name < b.name;
  //   });

  //   const auto& object1VariableIndexes = state.objectData[0].getVariableIndexes(stateMapping);
  //   ASSERT_EQ(state.objectData[0].name, "object1");
  //   ASSERT_EQ(state.objectData[0].variables.size(), 1);
  //   ASSERT_EQ(state.objectData[0].getVariableValue(object1VariableIndexes, "test_param1"), 20);

  //   const auto& object2VariableIndexes = state.objectData[1].getVariableIndexes(stateMapping);
  //   ASSERT_EQ(state.objectData[1].name, "object2");
  //   ASSERT_EQ(state.objectData[1].variables.size(), 1);
  //   ASSERT_EQ(state.objectData[1].getVariableValue(object2VariableIndexes, "test_param2"), 5);

  //   const auto& object3VariableIndexes = state.objectData[2].getVariableIndexes(stateMapping);
  //   ASSERT_EQ(state.objectData[2].name, "object3");
  //   ASSERT_EQ(state.objectData[2].variables.size(), 1);
  //   ASSERT_EQ(state.objectData[2].getVariableValue(object3VariableIndexes, "test_param3"), 12);

  //   ASSERT_EQ(state.delayedActionData.size(), 2);
  //   ASSERT_EQ(state.delayedActionData.top().actionName, "action2");
  //   ASSERT_EQ(state.delayedActionData.top().orientationVector, glm::ivec2(1, 0));
  //   ASSERT_EQ(state.delayedActionData.top().originatingPlayerId, 1);
  //   ASSERT_EQ(state.delayedActionData.top().priority, 113);
  //   ASSERT_EQ(state.delayedActionData.top().sourceObjectIdx, object2Idx);
  //   ASSERT_EQ(state.delayedActionData.top().vectorToDest, glm::ivec2(4, 5));
  //   state.delayedActionData.pop();
  //   ASSERT_EQ(state.delayedActionData.top().actionName, "action1");
  //   ASSERT_EQ(state.delayedActionData.top().orientationVector, glm::ivec2(0, 1));
  //   ASSERT_EQ(state.delayedActionData.top().originatingPlayerId, 0);
  //   ASSERT_EQ(state.delayedActionData.top().priority, 311);
  //   ASSERT_EQ(state.delayedActionData.top().sourceObjectIdx, object1Idx);
  //   ASSERT_EQ(state.delayedActionData.top().vectorToDest, glm::ivec2(3, 4));
  //   state.delayedActionData.pop();
}  // namespace griddly

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

  auto mockPlayerDefaultEmptyObject = mockObject("_empty", ' ', 1, 0, {-1, -1}, DiscreteOrientation(), {}, {});
  auto mockPlayerDefaultBoundaryObject = mockObject("_boundary", ' ', 1, 0, {-1, -1}, DiscreteOrientation(), {}, {});

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

  EXPECT_CALL(*mockGridPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  DelayedActionQueue delayedActionQueue;

  EXPECT_CALL(*mockGridPtr, getDelayedActions)
      .WillRepeatedly(ReturnRef(delayedActionQueue));

  std::map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions = {
      {"object1", std::make_shared<ObjectDefinition>(ObjectDefinition{"object1", 'a'})},
      {"object2", std::make_shared<ObjectDefinition>(ObjectDefinition{"object2", 'b'})},
      {"object3", std::make_shared<ObjectDefinition>(ObjectDefinition{"object3", 'c'})},
  };

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions()).WillRepeatedly(ReturnRefOfCopy(mockObjectDefinitions));
  EXPECT_CALL(*mockObjectGeneratorPtr, cloneInstance(Eq(mockObject1), _)).WillRepeatedly(Return(clonedMockObject1));
  EXPECT_CALL(*mockObjectGeneratorPtr, cloneInstance(Eq(mockObject2), _)).WillRepeatedly(Return(clonedMockObject2));
  EXPECT_CALL(*mockObjectGeneratorPtr, cloneInstance(Eq(mockObject3), _)).WillRepeatedly(Return(clonedMockObject3));
  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_empty"), _, _)).WillRepeatedly(Return(mockPlayerDefaultEmptyObject));
  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq("_boundary"), _, _)).WillRepeatedly(Return(mockPlayerDefaultBoundaryObject));

  EXPECT_CALL(*mockGDYFactoryPtr, getObjectGenerator()).WillRepeatedly(Return(mockObjectGeneratorPtr));

  auto gameProcessPtr = std::make_shared<TurnBasedGameProcess>("NONE", mockGDYFactoryPtr, mockGridPtr);

  for (int i = 0; i < 100; i++) {
    auto clonedPtr = gameProcessPtr->clone();
  }
}

}  // namespace griddly
