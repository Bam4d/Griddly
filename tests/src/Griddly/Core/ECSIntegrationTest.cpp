#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/Grid.hpp"
#include "Griddly/Core/Observers/Vulkan/VulkanObserver.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "Griddly/Core/TurnBasedGameProcess.hpp"

namespace griddly {

// End-to-end: a real GDY environment running on the ECS-backed Grid, where
// object variables are aliased into the arena and save/load/reset are
// single memcpys of the state block.
class ECSIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    grid_ = std::make_shared<Grid>();
    auto objectGenerator = std::make_shared<ObjectGenerator>();
    auto terminationGenerator = std::make_shared<TerminationGenerator>();
    gdyFactory_ = std::make_shared<GDYFactory>(objectGenerator, terminationGenerator, ResourceConfig{});
    gdyFactory_->initializeFromFile("tests/resources/ecs_test.yaml");
    gameProcess_ = std::make_shared<TurnBasedGameProcess>("Block2D", gdyFactory_, grid_);
    gameProcess_->addPlayer(std::make_shared<Player>(1, "p1", "Block2D", gameProcess_));
    gameProcess_->init();  // loads the level and captures the initial ECS state
  }

  std::shared_ptr<Grid> grid_;
  std::shared_ptr<GDYFactory> gdyFactory_;
  std::shared_ptr<TurnBasedGameProcess> gameProcess_;
};

TEST_F(ECSIntegrationTest, ObjectsAreBoundToTheArena) {
  auto world = grid_->getECSWorld();
  ASSERT_NE(world, nullptr);

  const auto& objects = grid_->getObjects();
  ASSERT_FALSE(objects.empty());

  for (const auto& object : objects) {
    EXPECT_TRUE(object->isECSBound()) << "object not bound: " << object->getObjectName();
    ASSERT_TRUE(world->isAlive(object->getEntityId()));
    // Arena location matches the object location.
    EXPECT_EQ(world->x(object->getEntityId()), object->getLocation().x);
    EXPECT_EQ(world->y(object->getEntityId()), object->getLocation().y);
  }
}

TEST_F(ECSIntegrationTest, VariableWritesThroughObjectLandInTheArena) {
  auto world = grid_->getECSWorld();
  std::shared_ptr<Object> object;
  for (const auto& o : grid_->getObjects()) {
    if (o->getObjectName() == "avatar") object = o;
  }
  ASSERT_NE(object, nullptr);
  const auto entity = object->getEntityId();

  const int32_t originalX = world->x(entity);
  *object->getVariableValue("_x") = originalX + 1000;  // write through the object handle
  EXPECT_EQ(world->x(entity), originalX + 1000);        // visible directly in the arena

  // Custom (GDY-defined) variables are aliased too: score sits at index 6.
  *object->getVariableValue("score") = 42;
  EXPECT_EQ(world->variable(entity, 6), 42);
}

TEST_F(ECSIntegrationTest, GridSaveLoadRoundTripsTheStateBlock) {
  auto world = grid_->getECSWorld();
  auto object = *grid_->getObjects().begin();
  const auto entity = object->getEntityId();

  std::vector<std::byte> blob(world->arenaSize());
  ASSERT_EQ(grid_->saveStateTo(blob.data()), world->arenaSize());
  const uint64_t hashBefore = world->computeHash();

  // Mutate: move the entity in the arena and bump the tick.
  world->x(entity) = 4242;
  world->setTick(9999);
  ASSERT_NE(world->computeHash(), hashBefore);

  ASSERT_TRUE(grid_->loadStateFrom(blob.data(), blob.size()));
  EXPECT_EQ(world->x(entity), object->getLocation().x);  // restored
  EXPECT_EQ(world->computeHash(), hashBefore);
  // The object handle is aliased to the arena, so it sees the restored value too.
  EXPECT_EQ(*object->getVariableValue("_x"), object->getLocation().x);
}

TEST_F(ECSIntegrationTest, InstantResetRestoresCapturedLevel) {
  auto world = grid_->getECSWorld();
  auto object = *grid_->getObjects().begin();
  const auto entity = object->getEntityId();
  const int32_t initialX = world->x(entity);
  const uint64_t initialHash = world->computeHash();

  world->x(entity) = -777;
  world->setTick(555);
  grid_->resetToInitialState();  // one memcpy

  EXPECT_EQ(world->x(entity), initialX);
  EXPECT_EQ(world->tick(), 0);
  EXPECT_EQ(world->computeHash(), initialHash);
}

}  // namespace griddly
