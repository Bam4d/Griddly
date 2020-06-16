#include "Griddly/Core/TurnBasedGameProcess.cpp"
#include "Mocks/Griddly/Core/GDY/MockGDYFactory.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "Mocks/Griddly/Core/LevelGenerators/MockMapReader.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
#include "Mocks/Griddly/Core/Observers/MockObserver.cpp"
#include "Mocks/Griddly/Core/Players/MockPlayer.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::AnyNumber;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;

namespace griddly {

  MATCHER_P(PlayerObserverDefinitionEqMatcher, definition, "") {
  auto isEqual = definition.gridHeight == arg.gridHeight &&
                 definition.gridWidth == arg.gridWidth &&
                 definition.gridXOffset == arg.gridXOffset &&
                 definition.gridYOffset == arg.gridYOffset &&
                 definition.rotateWithAvatar == arg.rotateWithAvatar &&
                 definition.trackAvatar == arg.trackAvatar;

  return isEqual;
}

MATCHER_P(ObserverConfigEqMatcher, definition, "") {
  auto isEqual = definition.gridHeight == arg.gridHeight &&
                 definition.gridWidth == arg.gridWidth &&
                 definition.gridXOffset == arg.gridXOffset &&
                 definition.gridYOffset == arg.gridYOffset &&
                 definition.rotateWithAvatar == arg.rotateWithAvatar;
  return isEqual;
}

std::shared_ptr<MockPlayer> mockPlayer(std::string playerName, uint32_t playerId, std::shared_ptr<GameProcess> gameProcessPtr, std::shared_ptr<MockObject> mockPlayerAvatarPtr) {
  auto mockPlayerPtr = std::shared_ptr<MockPlayer>(new MockPlayer());
  EXPECT_CALL(*mockPlayerPtr, getName)
      .WillRepeatedly(Return(playerName));
  EXPECT_CALL(*mockPlayerPtr, getId)
      .WillRepeatedly(Return(playerId));

  auto playerObserverDefinition = PlayerObserverDefinition{4, 8, 0, 0, false, false};

  EXPECT_CALL(*mockPlayerPtr, init(PlayerObserverDefinitionEqMatcher(playerObserverDefinition), Eq(gameProcessPtr)))
      .Times(1);

  if (mockPlayerAvatarPtr != nullptr) {
    EXPECT_CALL(*mockPlayerPtr, setAvatar(Eq(mockPlayerAvatarPtr)))
        .Times(AnyNumber());
  }

  return mockPlayerPtr;
}

TEST(GameProcessTest, init) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));
    
  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillOnce(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initAlreadyInitialized) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));
  
  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillOnce(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_THROW(gameProcessPtr->init(), std::runtime_error);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initNoLevelGenerator_DirectControl) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(nullptr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillOnce(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, nullptr);


  gameProcessPtr->addPlayer(mockPlayerPtr);

  ASSERT_THROW(gameProcessPtr->init(), std::invalid_argument);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
}

TEST(GameProcessTest, initNoLevelGenerator_SelectiveControl) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(nullptr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));

  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillOnce(Return(ActionControlScheme::SELECTION_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, nullptr);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
}

TEST(GameProcessTest, initNoGlobalObserver) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, nullptr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));
    
  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillOnce(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr);

  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillOnce(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initNoPlayerObserverDefinition) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{0, 0, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));
    
  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillOnce(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = std::shared_ptr<MockPlayer>(new MockPlayer());
  EXPECT_CALL(*mockPlayerPtr, getName)
      .WillRepeatedly(Return("Bob"));
  EXPECT_CALL(*mockPlayerPtr, getId)
      .WillRepeatedly(Return(1));

  auto playerObserverDefinition = PlayerObserverDefinition{10, 10, 0, 0, false, false};

  EXPECT_CALL(*mockPlayerPtr, init(PlayerObserverDefinitionEqMatcher(playerObserverDefinition), Eq(gameProcessPtr)))
      .Times(1);

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
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, initWrongNumberOfPlayers) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillOnce(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillOnce(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(1);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillOnce(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(10));

  auto mockPlayerPtr = std::shared_ptr<MockPlayer>(new MockPlayer());

  gameProcessPtr->addPlayer(mockPlayerPtr);

  ASSERT_THROW(gameProcessPtr->init(), std::invalid_argument);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, reset) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillRepeatedly(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(2);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillRepeatedly(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  auto observerConfig = ObserverConfig{10, 10, 0, 0, false};
  EXPECT_CALL(*mockObserverPtr, init(ObserverConfigEqMatcher(observerConfig)))
      .Times(1);

  auto mockObservationPtr = std::shared_ptr<uint8_t>(new uint8_t[3] {0,1,2});
  
  EXPECT_CALL(*mockObserverPtr, reset())
      .WillOnce(Return(mockObservationPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));
    
  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillRepeatedly(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr);
  EXPECT_CALL(*mockPlayerPtr, reset())
      .Times(1);


  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillRepeatedly(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  auto observation = gameProcessPtr->reset();

  auto resetObservationPointer = std::vector<uint8_t>(observation.get(), observation.get() + 3);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(mockObservationPtr.get(), 3));

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  ASSERT_TRUE(gameProcessPtr->isStarted());
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, resetNotInitialized) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  ASSERT_THROW(gameProcessPtr->reset(), std::runtime_error);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

// This is not really supported at the moment so not much point testing for it
TEST(GameProcessTest, resetNoLevelGenerator) {
}


TEST(GameProcessTest, resetNoGlobalObserver) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, nullptr, mockGDYFactoryPtr));

  auto mockMapReaderPtr = std::shared_ptr<MockMapReader>(new MockMapReader());

  EXPECT_CALL(*mockGDYFactoryPtr, getLevelGenerator())
      .WillRepeatedly(Return(mockMapReaderPtr));

  EXPECT_CALL(*mockGDYFactoryPtr, getGlobalVariableDefinitions())
      .WillRepeatedly(Return(std::unordered_map<std::string, int32_t>{}));

  EXPECT_CALL(*mockGridPtr, resetGlobalVariables)
      .Times(2);

  auto mockPlayerAvatarPtr = std::shared_ptr<MockObject>(new MockObject());

  EXPECT_CALL(*mockMapReaderPtr, reset(Eq(mockGridPtr)))
      .WillRepeatedly(Return(std::unordered_map<uint32_t, std::shared_ptr<Object>>{{1, mockPlayerAvatarPtr}}));

  EXPECT_CALL(*mockGridPtr, getHeight)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGridPtr, getWidth)
      .WillRepeatedly(Return(10));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerObserverDefinition())
      .WillOnce(Return(PlayerObserverDefinition{4, 8, 0, 0, false, false}));

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(1));
    
  EXPECT_CALL(*mockGDYFactoryPtr, getActionControlScheme())
    .WillRepeatedly(Return(ActionControlScheme::DIRECT_ABSOLUTE));

  auto mockPlayerPtr = mockPlayer("Bob", 1, gameProcessPtr, mockPlayerAvatarPtr);
  EXPECT_CALL(*mockPlayerPtr, reset())
      .Times(1);


  EXPECT_CALL(*mockGDYFactoryPtr, createTerminationHandler)
      .WillRepeatedly(Return(nullptr));

  gameProcessPtr->addPlayer(mockPlayerPtr);

  gameProcessPtr->init();

  auto observation = gameProcessPtr->reset();

  ASSERT_EQ(observation, nullptr);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 1);
  ASSERT_TRUE(gameProcessPtr->isStarted());
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockMapReaderPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockPlayerAvatarPtr.get()));
}

TEST(GameProcessTest, observe) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockObservationPtr = std::shared_ptr<uint8_t>(new uint8_t[3] {0,1,2});
  
  EXPECT_CALL(*mockObserverPtr, update(0))
      .WillOnce(Return(mockObservationPtr));

  auto observation = gameProcessPtr->observe(0);
  auto resetObservationPointer = std::vector<uint8_t>(observation.get(), observation.get() + 3);

  ASSERT_THAT(resetObservationPointer, ElementsAreArray(mockObservationPtr.get(), 3));

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

TEST(GameProcessTest, addPlayer) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockPlayerPtr1 = mockPlayer("Bob", 1, gameProcessPtr, nullptr);
  auto mockPlayerPtr2 = mockPlayer("Alice", 2, gameProcessPtr, nullptr);
  auto mockPlayerPtr3 = mockPlayer("Obama", 3, gameProcessPtr, nullptr);

  EXPECT_CALL(*mockGDYFactoryPtr, getPlayerCount())
      .WillRepeatedly(Return(3));

  gameProcessPtr->addPlayer(mockPlayerPtr1);
  gameProcessPtr->addPlayer(mockPlayerPtr2);
  gameProcessPtr->addPlayer(mockPlayerPtr3);

  ASSERT_EQ(gameProcessPtr->getNumPlayers(), 3);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

TEST(GameProcessTest, addTooManyPlayers) {
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockObserverPtr = std::shared_ptr<MockObserver>(new MockObserver(mockGridPtr));
  auto mockGDYFactoryPtr = std::shared_ptr<MockGDYFactory>(new MockGDYFactory());
  auto gameProcessPtr = std::shared_ptr<TurnBasedGameProcess>(new TurnBasedGameProcess(mockGridPtr, mockObserverPtr, mockGDYFactoryPtr));

  auto mockPlayerPtr1 = mockPlayer("Bob", 1, gameProcessPtr, nullptr);
  auto mockPlayerPtr2 = mockPlayer("Alice", 2, gameProcessPtr, nullptr);
  auto mockPlayerPtr3 = std::shared_ptr<MockPlayer>(new MockPlayer());
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
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObserverPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGDYFactoryPtr.get()));
}

}  // namespace griddly