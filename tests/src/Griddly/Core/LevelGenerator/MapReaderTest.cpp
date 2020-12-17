#include <iostream>
#include <memory>

#include "Griddly/Core/LevelGenerators/MapReader.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObject.cpp"
#include "Mocks/Griddly/Core/GDY/Objects/MockObjectGenerator.cpp"
#include "Mocks/Griddly/Core/MockGrid.cpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
using ::testing::_;
using ::testing::ByMove;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace griddly {

std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions(std::vector<std::string> objectNames) {
  std::unordered_map<std::string, std::shared_ptr<ObjectDefinition>> mockObjectDefinitions;
  for (auto name : objectNames) {
    ObjectDefinition objectDefinition = {
        name};
    mockObjectDefinitions[name] = std::make_shared<ObjectDefinition>(objectDefinition);
  }

  return mockObjectDefinitions;
}

TEST(MapReaderTest, testLoadStringWithPlayerObjects) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockAvatarObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string avatarObjectName = "avatar";

  auto objectDefinitions = mockObjectDefinitions({wallObjectName, avatarObjectName});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(Return(objectDefinitions));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(wallObjectName)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(avatarObjectName)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(8)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(avatarObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), Eq(0), _))
      .Times(8)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(avatarObjectName), Eq(1), _))
      .Times(1)
      .WillRepeatedly(Return(mockAvatarObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(3), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockWallObject), Eq(true)))
      .Times(8);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockAvatarObject), Eq(true)))
      .Times(1);

  std::string levelString = "W   W   W\nW   P1  W\nW   W   W";
  auto levelStringStream = std::stringstream(levelString);

  mapReader->parseFromStream(levelStringStream);
  mapReader->reset(mockGridPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(MapReaderTest, testLoadStringWithPlayerObjectsRandomWhitespace) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockAvatarObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string avatarObjectName = "avatar";

  auto objectDefinitions = mockObjectDefinitions({wallObjectName, avatarObjectName});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(Return(objectDefinitions));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(wallObjectName)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(avatarObjectName)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(8)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(avatarObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), Eq(0), _))
      .Times(8)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(avatarObjectName), Eq(1), _))
      .Times(1)
      .WillRepeatedly(Return(mockAvatarObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(3), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockWallObject), Eq(true)))
      .Times(8);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockAvatarObject), Eq(true)))
      .Times(1);

  std::string levelString = "W  W  W \nW  P1  W\t\nW\tW\tW\n";
  auto levelStringStream = std::stringstream(levelString);

  mapReader->parseFromStream(levelStringStream);
  mapReader->reset(mockGridPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(MapReaderTest, testLoadStringNoSpaces) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockAvatarObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string avatarObjectName = "avatar";

  auto objectDefinitions = mockObjectDefinitions({wallObjectName, avatarObjectName});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(Return(objectDefinitions));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(wallObjectName)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(avatarObjectName)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(8)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(avatarObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), Eq(0), _))
      .Times(8)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(avatarObjectName), Eq(1), _))
      .Times(1)
      .WillRepeatedly(Return(mockAvatarObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(3), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockWallObject), Eq(true)))
      .Times(8);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockAvatarObject), Eq(true)))
      .Times(1);
  std::string levelString = "WWW\nWP1W\nWWW";
  auto levelStringStream = std::stringstream(levelString);

  mapReader->parseFromStream(levelStringStream);
  mapReader->reset(mockGridPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

TEST(MapReaderTest, testLoadStringNoSpacesWithDots) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockAvatarObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string avatarObjectName = "avatar";

  auto objectDefinitions = mockObjectDefinitions({wallObjectName, avatarObjectName});

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectDefinitions())
      .WillRepeatedly(Return(objectDefinitions));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(wallObjectName)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(avatarObjectName)))
      .Times(1);

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(12)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(avatarObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), Eq(0), _))
      .Times(12)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(avatarObjectName), Eq(1), _))
      .Times(1)
      .WillRepeatedly(Return(mockAvatarObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(5), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockWallObject), Eq(true)))
      .Times(12);

  EXPECT_CALL(*mockGridPtr, addObject(_, Eq(mockAvatarObject), Eq(true)))
      .Times(1);
  std::string levelString = "WWWWW\nW.P1.W\nWWWWW";
  auto levelStringStream = std::stringstream(levelString);

  mapReader->parseFromStream(levelStringStream);
  mapReader->reset(mockGridPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

}  // namespace griddly