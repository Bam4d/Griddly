#include "Griddly/Core/SpatialHashCollisionDetector.cpp"
#include "Griddly/Core/TestUtils/common.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "spdlog/spdlog.h"

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::AnyOf;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::ReturnRefOfCopy;
using ::testing::UnorderedElementsAre;

namespace griddly {

std::shared_ptr<MockObject> mockObject(std::string objectName, glm::ivec2 location, DiscreteOrientation orientation) {
  return mockObject(objectName, '?', 1, 0, location, orientation);
}
std::shared_ptr<MockObject> mockObject(std::string objectName, glm::ivec2 location) {
  return mockObject(objectName, '?', 1, 0, location);
}

TEST(SpatialHashCollisionDetectorTest, test_upsert_object) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 10, 3, TriggerType::RANGE_BOX_AREA));

  auto mockObjectPtr = mockObject("object", {0, 0});

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr));

  EXPECT_CALL(*mockObjectPtr, getLocation()).WillRepeatedly(ReturnRefOfCopy(glm::ivec2(1, 0)));

  ASSERT_FALSE(collisionDetector->upsert(mockObjectPtr));

  EXPECT_CALL(*mockObjectPtr, getLocation()).WillRepeatedly(ReturnRefOfCopy(glm::ivec2(1, 0)));

  ASSERT_FALSE(collisionDetector->upsert(mockObjectPtr));
}

TEST(SpatialHashCollisionDetectorTest, test_remove_object) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 10, 3, TriggerType::RANGE_BOX_AREA));

  auto mockObjectPtr = mockObject("object", {0, 0});

  ASSERT_FALSE(collisionDetector->remove(mockObjectPtr));

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr));

  ASSERT_TRUE(collisionDetector->remove(mockObjectPtr));
}

TEST(SpatialHashCollisionDetectorTest, test_search_area_single_hash) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 10, 2, TriggerType::RANGE_BOX_AREA));

  auto mockObjectPtr1 = mockObject("object1", {1, 1});
  auto mockObjectPtr2 = mockObject("object2", {3, 1});
  auto mockObjectPtr3 = mockObject("object3", {1, 3});
  auto mockObjectPtr4 = mockObject("object4", {3, 3});
  auto mockObjectPtr5 = mockObject("object5", {0, 0});
  auto mockObjectPtr6 = mockObject("object6", {4, 0});
  auto mockObjectPtr7 = mockObject("object7", {0, 4});
  auto mockObjectPtr8 = mockObject("object8", {4, 4});

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr1));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr2));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr3));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr4));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr5));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr6));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr7));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr8));

  auto searchResults1 = collisionDetector->search({3, 3});
  auto searchResults2 = collisionDetector->search({2, 2});
  auto searchResults3 = collisionDetector->search({1, 1});

  auto objects1 = searchResults1.objectSet;
  auto objects2 = searchResults2.objectSet;
  auto objects3 = searchResults3.objectSet;

  ASSERT_THAT(objects1, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3, mockObjectPtr4, mockObjectPtr8));
  ASSERT_THAT(objects2, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3, mockObjectPtr4, mockObjectPtr5, mockObjectPtr6, mockObjectPtr7, mockObjectPtr8));
  ASSERT_THAT(objects3, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3, mockObjectPtr4, mockObjectPtr5));

  auto closestObject1 = searchResults1.closestObjects.top().target;
  auto closestObject2 = searchResults2.closestObjects.top().target;
  auto closestObject3 = searchResults3.closestObjects.top().target;

  ASSERT_EQ(closestObject1, mockObjectPtr4);
  ASSERT_THAT(closestObject2, AnyOf(Eq(mockObjectPtr1), Eq(mockObjectPtr2), Eq(mockObjectPtr3), Eq(mockObjectPtr4)));
  ASSERT_EQ(closestObject3, mockObjectPtr1);
}

TEST(SpatialHashCollisionDetectorTest, test_search_area_across_many_hash) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 4, 2, TriggerType::RANGE_BOX_AREA));

  auto mockObjectPtr1 = mockObject("object1", {1, 1});
  auto mockObjectPtr2 = mockObject("object2", {3, 1});
  auto mockObjectPtr3 = mockObject("object3", {1, 3});
  auto mockObjectPtr4 = mockObject("object4", {3, 3});
  auto mockObjectPtr5 = mockObject("object5", {0, 0});
  auto mockObjectPtr6 = mockObject("object6", {4, 0});
  auto mockObjectPtr7 = mockObject("object7", {0, 4});
  auto mockObjectPtr8 = mockObject("object8", {4, 4});

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr1));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr2));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr3));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr4));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr5));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr6));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr7));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr8));

  auto searchResults1 = collisionDetector->search({3, 3});
  auto searchResults2 = collisionDetector->search({2, 2});
  auto searchResults3 = collisionDetector->search({1, 1});

  auto objects1 = searchResults1.objectSet;
  auto objects2 = searchResults2.objectSet;
  auto objects3 = searchResults3.objectSet;

  ASSERT_THAT(objects1, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3, mockObjectPtr4, mockObjectPtr8));
  ASSERT_THAT(objects2, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3, mockObjectPtr4, mockObjectPtr5, mockObjectPtr6, mockObjectPtr7, mockObjectPtr8));
  ASSERT_THAT(objects3, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3, mockObjectPtr4, mockObjectPtr5));

  auto closestObject1 = searchResults1.closestObjects.top().target;
  auto closestObject2 = searchResults2.closestObjects.top().target;
  auto closestObject3 = searchResults3.closestObjects.top().target;

  ASSERT_EQ(closestObject1, mockObjectPtr4);
  ASSERT_THAT(closestObject2, AnyOf(Eq(mockObjectPtr1), Eq(mockObjectPtr2), Eq(mockObjectPtr3), Eq(mockObjectPtr4)));
  ASSERT_EQ(closestObject3, mockObjectPtr1);
}

TEST(SpatialHashCollisionDetectorTest, test_search_boundary_single_hash) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 10, 2, TriggerType::RANGE_BOX_BOUNDARY));

  auto mockObjectPtr1 = mockObject("object1", {1, 1});
  auto mockObjectPtr2 = mockObject("object2", {3, 1});
  auto mockObjectPtr3 = mockObject("object3", {1, 3});
  auto mockObjectPtr4 = mockObject("object4", {3, 3});
  auto mockObjectPtr5 = mockObject("object5", {0, 0});
  auto mockObjectPtr6 = mockObject("object6", {4, 0});
  auto mockObjectPtr7 = mockObject("object7", {0, 4});
  auto mockObjectPtr8 = mockObject("object8", {4, 4});

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr1));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr2));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr3));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr4));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr5));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr6));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr7));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr8));

  auto searchResults1 = collisionDetector->search({3, 3});
  auto searchResults2 = collisionDetector->search({2, 2});
  auto searchResults3 = collisionDetector->search({1, 1});

  auto objects1 = searchResults1.objectSet;
  auto objects2 = searchResults2.objectSet;
  auto objects3 = searchResults3.objectSet;

  ASSERT_THAT(objects1, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3));
  ASSERT_THAT(objects2, UnorderedElementsAre(mockObjectPtr5, mockObjectPtr6, mockObjectPtr7, mockObjectPtr8));
  ASSERT_THAT(objects3, UnorderedElementsAre(mockObjectPtr2, mockObjectPtr3, mockObjectPtr4));

  auto closestObject1 = searchResults1.closestObjects.top().target;
  auto closestObject2 = searchResults2.closestObjects.top().target;
  auto closestObject3 = searchResults3.closestObjects.top().target;

  ASSERT_THAT(closestObject1, AnyOf(Eq(mockObjectPtr2), Eq(mockObjectPtr3)));
  ASSERT_THAT(closestObject2, AnyOf(Eq(mockObjectPtr5), Eq(mockObjectPtr6), Eq(mockObjectPtr7), Eq(mockObjectPtr8)));
  ASSERT_THAT(closestObject3, AnyOf(Eq(mockObjectPtr2), Eq(mockObjectPtr3)));
}

TEST(SpatialHashCollisionDetectorTest, test_search_boundary_across_many_hash) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 4, 2, TriggerType::RANGE_BOX_BOUNDARY));

  auto mockObjectPtr1 = mockObject("object1", {1, 1});
  auto mockObjectPtr2 = mockObject("object2", {3, 1});
  auto mockObjectPtr3 = mockObject("object3", {1, 3});
  auto mockObjectPtr4 = mockObject("object4", {3, 3});
  auto mockObjectPtr5 = mockObject("object5", {0, 0});
  auto mockObjectPtr6 = mockObject("object6", {4, 0});
  auto mockObjectPtr7 = mockObject("object7", {0, 4});
  auto mockObjectPtr8 = mockObject("object8", {4, 4});

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr1));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr2));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr3));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr4));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr5));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr6));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr7));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr8));

  auto searchResults1 = collisionDetector->search({3, 3});
  auto searchResults2 = collisionDetector->search({2, 2});
  auto searchResults3 = collisionDetector->search({1, 1});

  auto objects1 = searchResults1.objectSet;
  auto objects2 = searchResults2.objectSet;
  auto objects3 = searchResults3.objectSet;

  ASSERT_THAT(objects1, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3));
  ASSERT_THAT(objects2, UnorderedElementsAre(mockObjectPtr5, mockObjectPtr6, mockObjectPtr7, mockObjectPtr8));
  ASSERT_THAT(objects3, UnorderedElementsAre(mockObjectPtr2, mockObjectPtr3, mockObjectPtr4));

  auto closestObject1 = searchResults1.closestObjects.top().target;
  auto closestObject2 = searchResults2.closestObjects.top().target;
  auto closestObject3 = searchResults3.closestObjects.top().target;

  ASSERT_THAT(closestObject1, AnyOf(Eq(mockObjectPtr2), Eq(mockObjectPtr3)));
  ASSERT_THAT(closestObject2, AnyOf(Eq(mockObjectPtr5), Eq(mockObjectPtr6), Eq(mockObjectPtr7), Eq(mockObjectPtr8)));
  ASSERT_THAT(closestObject3, AnyOf(Eq(mockObjectPtr2), Eq(mockObjectPtr3)));
}

TEST(SpatialHashCollisionDetectorTest, test_search_range_larger_than_cell_size) {
  auto collisionDetector = std::shared_ptr<CollisionDetector>(new SpatialHashCollisionDetector(10, 10, 1, 2, TriggerType::RANGE_BOX_BOUNDARY));

  auto mockObjectPtr1 = mockObject("object1", {1, 1});
  auto mockObjectPtr2 = mockObject("object2", {3, 1});
  auto mockObjectPtr3 = mockObject("object3", {1, 3});
  auto mockObjectPtr4 = mockObject("object4", {3, 3});
  auto mockObjectPtr5 = mockObject("object5", {0, 0});
  auto mockObjectPtr6 = mockObject("object6", {4, 0});
  auto mockObjectPtr7 = mockObject("object7", {0, 4});
  auto mockObjectPtr8 = mockObject("object8", {4, 4});

  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr1));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr2));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr3));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr4));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr5));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr6));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr7));
  ASSERT_TRUE(collisionDetector->upsert(mockObjectPtr8));

  auto searchResults1 = collisionDetector->search({3, 3});
  auto searchResults2 = collisionDetector->search({2, 2});
  auto searchResults3 = collisionDetector->search({1, 1});

  auto objects1 = searchResults1.objectSet;
  auto objects2 = searchResults2.objectSet;
  auto objects3 = searchResults3.objectSet;

  ASSERT_THAT(objects1, UnorderedElementsAre(mockObjectPtr1, mockObjectPtr2, mockObjectPtr3));
  ASSERT_THAT(objects2, UnorderedElementsAre(mockObjectPtr5, mockObjectPtr6, mockObjectPtr7, mockObjectPtr8));
  ASSERT_THAT(objects3, UnorderedElementsAre(mockObjectPtr2, mockObjectPtr3, mockObjectPtr4));

  auto closestObject1 = searchResults1.closestObjects.top().target;
  auto closestObject2 = searchResults2.closestObjects.top().target;
  auto closestObject3 = searchResults3.closestObjects.top().target;

  ASSERT_THAT(closestObject1, AnyOf(Eq(mockObjectPtr2), Eq(mockObjectPtr3)));
  ASSERT_THAT(closestObject2, AnyOf(Eq(mockObjectPtr5), Eq(mockObjectPtr6), Eq(mockObjectPtr7), Eq(mockObjectPtr8)));
  ASSERT_THAT(closestObject3, AnyOf(Eq(mockObjectPtr2), Eq(mockObjectPtr3)));
}

}  // namespace griddly