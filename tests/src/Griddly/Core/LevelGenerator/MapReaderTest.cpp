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

TEST(MapReaderTest, testLoadStringWithPlayerObjects) {
  auto mockObjectGeneratorPtr = std::shared_ptr<MockObjectGenerator>(new MockObjectGenerator());
  auto mockGridPtr = std::shared_ptr<MockGrid>(new MockGrid());
  auto mockWallObject = std::shared_ptr<MockObject>(new MockObject());
  auto mockPlayerObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string playerObjectName = "player";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(8)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(playerObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), _))
      .Times(8)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(playerObjectName), _))
      .Times(1)
      .WillRepeatedly(Return(mockPlayerObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(3), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getGlobalParameters())
      .Times(9)
      .WillRepeatedly(Return(std::unordered_map<std::string, std::shared_ptr<int32_t>>{}));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(0), _, Eq(mockWallObject)))
      .Times(8);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(1), _, Eq(mockPlayerObject)))
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
  auto mockPlayerObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string playerObjectName = "player";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(8)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(playerObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), _))
      .Times(8)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(playerObjectName), _))
      .Times(1)
      .WillRepeatedly(Return(mockPlayerObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(3), Eq(3)))
      .Times(1);
    
  EXPECT_CALL(*mockGridPtr, getGlobalParameters())
      .Times(9)
      .WillRepeatedly(Return(std::unordered_map<std::string, std::shared_ptr<int32_t>>{}));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(0), _, Eq(mockWallObject)))
      .Times(8);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(1), _, Eq(mockPlayerObject)))
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
  auto mockPlayerObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string playerObjectName = "player";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(8)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(playerObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), _))
      .Times(8)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(playerObjectName), _))
      .Times(1)
      .WillRepeatedly(Return(mockPlayerObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(3), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getGlobalParameters())
      .Times(9)
      .WillRepeatedly(Return(std::unordered_map<std::string, std::shared_ptr<int32_t>>{}));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(0), _, Eq(mockWallObject)))
      .Times(8);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(1), _, Eq(mockPlayerObject)))
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
  auto mockPlayerObject = std::shared_ptr<MockObject>(new MockObject());
  std::shared_ptr<MapReader> mapReader(new MapReader(mockObjectGeneratorPtr));

  std::string wallObjectName = "wall";
  std::string playerObjectName = "player";

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('W')))
      .Times(12)
      .WillRepeatedly(ReturnRef(wallObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, getObjectNameFromMapChar(Eq('P')))
      .Times(1)
      .WillRepeatedly(ReturnRef(playerObjectName));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(wallObjectName), _))
      .Times(12)
      .WillRepeatedly(Return(mockWallObject));

  EXPECT_CALL(*mockObjectGeneratorPtr, newInstance(Eq(playerObjectName), _))
      .Times(1)
      .WillRepeatedly(Return(mockPlayerObject));

  EXPECT_CALL(*mockGridPtr, resetMap(Eq(5), Eq(3)))
      .Times(1);

  EXPECT_CALL(*mockGridPtr, getGlobalParameters())
      .Times(13)
      .WillRepeatedly(Return(std::unordered_map<std::string, std::shared_ptr<int32_t>>{}));

  EXPECT_CALL(*mockGridPtr, initObject(Eq(0), _, Eq(mockWallObject)))
      .Times(12);

  EXPECT_CALL(*mockGridPtr, initObject(Eq(1), _, Eq(mockPlayerObject)))
      .Times(1);
  std::string levelString = "WWWWW\nW.P1.W\nWWWWW";
  auto levelStringStream = std::stringstream(levelString);

  mapReader->parseFromStream(levelStringStream);
  mapReader->reset(mockGridPtr);

  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockGridPtr.get()));
  EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockObjectGeneratorPtr.get()));
}

}  // namespace griddly