#include <gtest/gtest.h>

#include <vector>

#include "../../../../src/Griddly/Core/ECS/ECSWorld.hpp"

using namespace griddly;

static ECSWorldConfig testConfig() {
  ECSWorldConfig c;
  c.maxEntities = 64;
  c.maxVariablesPerEntity = 8;
  c.maxPlayers = 2;
  c.maxGlobalVariables = 16;
  c.gridWidth = 8;
  c.gridHeight = 8;
  c.maxDelayedActions = 16;
  return c;
}

TEST(ECSWorldTest, CreateAndDestroyRecyclesSlotsWithoutChurn) {
  ECSWorld world(testConfig());
  const size_t sizeBefore = world.arenaSize();

  EntityId a = world.createEntity();
  EntityId b = world.createEntity();
  EXPECT_TRUE(world.isAlive(a));
  EXPECT_TRUE(world.isAlive(b));
  EXPECT_EQ(world.aliveCount(), 2u);
  EXPECT_NE(a.index, b.index);

  world.destroyEntity(a);
  EXPECT_FALSE(world.isAlive(a));
  EXPECT_EQ(world.aliveCount(), 1u);

  // Recycled slot: same index, bumped generation (stale handle stays dead).
  EntityId c = world.createEntity();
  EXPECT_EQ(c.index, a.index);
  EXPECT_NE(c.generation, a.generation);
  EXPECT_FALSE(world.isAlive(a));
  EXPECT_TRUE(world.isAlive(c));

  // No reallocation happened: arena size is fixed.
  EXPECT_EQ(world.arenaSize(), sizeBefore);
}

TEST(ECSWorldTest, ComponentsAreStoredInTheArena) {
  ECSWorld world(testConfig());
  EntityId e = world.createEntity();
  world.objectTypeId(e) = 7;
  world.x(e) = 3;
  world.y(e) = 4;
  world.dx(e) = 0;
  world.dy(e) = -1;
  world.playerId(e) = 1;
  world.zIdx(e) = 2;
  world.variable(e, 6) = 42;

  EXPECT_EQ(world.objectTypeId(e), 7u);
  EXPECT_EQ(world.x(e), 3);
  EXPECT_EQ(world.y(e), 4);
  EXPECT_EQ(world.variable(e, 6), 42);
}

TEST(ECSWorldTest, SpatialHashTracksObjectsPerTile) {
  ECSWorld world(testConfig());
  EntityId wall = world.createEntity();
  world.zIdx(wall) = 0;
  world.placeEntity(wall, 1, 1);
  EntityId avatar = world.createEntity();
  world.zIdx(avatar) = 5;
  world.placeEntity(avatar, 1, 1);

  // Highest z-index is on top.
  EXPECT_EQ(world.topObjectAt(1, 1), avatar);

  world.moveEntity(avatar, 2, 1);
  EXPECT_EQ(world.topObjectAt(1, 1), wall);
  EXPECT_EQ(world.topObjectAt(2, 1), avatar);
}

TEST(ECSWorldTest, SaveLoadIsAChunkOfBytes) {
  ECSWorld world(testConfig());
  EntityId e = world.createEntity();
  world.objectTypeId(e) = 9;
  world.x(e) = 5; world.y(e) = 6;
  world.setTick(123);
  world.setGlobalVariableCount(4);
  world.globalVariable(2, 1) = -7;
  world.pushDelayedAction(DelayedActionRecord{10, 1, e, 3, 0, -1, 0, -1});

  std::vector<std::byte> blob(world.arenaSize());
  const size_t written = world.saveTo(blob.data());
  EXPECT_EQ(written, world.arenaSize());

  // Mutate everything.
  world.destroyEntity(e);
  world.setTick(999);
  world.globalVariable(2, 1) = 0;
  world.clearDelayedActions();
  EXPECT_NE(world.tick(), 123);

  // Load the blob back: full state restored by memcpy.
  ASSERT_TRUE(world.loadFrom(blob.data(), blob.size()));
  EXPECT_EQ(world.tick(), 123);
  EXPECT_EQ(world.globalVariable(2, 1), -7);
  EXPECT_EQ(world.delayedActionCount(), 1u);
  EXPECT_TRUE(world.isAlive(e));
  EXPECT_EQ(world.objectTypeId(e), 9u);
  EXPECT_EQ(world.x(e), 5);

  // Corrupt/mismatched blobs are rejected.
  EXPECT_FALSE(world.loadFrom(blob.data(), blob.size() - 1));
  blob[0] = std::byte{0xFF};
  EXPECT_FALSE(world.loadFrom(blob.data(), blob.size()));
}

TEST(ECSWorldTest, InstantResetRestoresCapturedInitialState) {
  ECSWorld world(testConfig());
  EntityId e = world.createEntity();
  world.x(e) = 2; world.y(e) = 2; world.objectTypeId(e) = 4;
  world.setTick(0);
  world.captureInitialState();
  const uint64_t initialHash = world.computeHash();

  // Play for a while: move, spawn, tick, queue actions.
  world.moveEntity(e, 7, 7);
  EntityId e2 = world.createEntity();
  world.setTick(500);
  world.pushDelayedAction(DelayedActionRecord{501, 1, e2, 1, 1, 0, 1, 0});
  EXPECT_NE(world.computeHash(), initialHash);

  world.resetToInitial();

  EXPECT_EQ(world.tick(), 0);
  EXPECT_EQ(world.x(e), 2);
  EXPECT_EQ(world.y(e), 2);
  EXPECT_FALSE(world.isAlive(e2));
  EXPECT_EQ(world.delayedActionCount(), 0u);
  EXPECT_EQ(world.computeHash(), initialHash);  // byte-identical state after reset
}

TEST(ECSWorldTest, DestroyUnlinksFromTileAndRecycledSlotStaysConsistent) {
  ECSWorld world(testConfig());
  EntityId a = world.createEntity();
  world.zIdx(a) = 0;
  world.placeEntity(a, 2, 2);
  EntityId b = world.createEntity();
  world.zIdx(b) = 5;
  world.placeEntity(b, 2, 2);
  EXPECT_EQ(world.topObjectAt(2, 2), b);

  world.destroyEntity(b);
  // Tile list must no longer reference the dead slot.
  EXPECT_EQ(world.topObjectAt(2, 2), a);

  // Recycle the slot onto the same tile; lists must stay acyclic/consistent.
  EntityId b2 = world.createEntity();
  EXPECT_EQ(b2.index, b.index);
  world.zIdx(b2) = 7;
  world.placeEntity(b2, 2, 2);
  EXPECT_EQ(world.topObjectAt(2, 2), b2);

  // Many moves of both entities must terminate and keep the top correct.
  for (int t = 0; t < 100; ++t) {
    world.moveEntity(b2, t % 8, (t * 3) % 8);
    world.moveEntity(a, (t * 5) % 8, t % 8);
  }
  SUCCEED();
}

TEST(ECSWorldTest, DestroyingNeverPlacedEntityDoesNotCorruptTileLists) {
  ECSWorld world(testConfig());
  EntityId a = world.createEntity();  // never placed on a tile
  EXPECT_EQ(world.topObjectAt(0, 0), NULL_ENTITY);
  world.destroyEntity(a);  // must not hang or corrupt occupancy
  EXPECT_EQ(world.topObjectAt(0, 0), NULL_ENTITY);
  EntityId b = world.createEntity();
  world.placeEntity(b, 0, 0);
  EXPECT_EQ(world.topObjectAt(0, 0), b);
}

TEST(ECSWorldTest, OutOfBoundsEntitiesAreNotSpatiallyHashed) {
  ECSWorld world(testConfig());  // 8x8 grid
  EntityId e = world.createEntity();
  world.placeEntity(e, -1, -1);  // e.g. the default "_empty" object location
  EXPECT_EQ(world.topObjectAt(-1, -1), NULL_ENTITY);
  EXPECT_EQ(world.topObjectAt(0, 0), NULL_ENTITY);
  world.destroyEntity(e);  // must not corrupt or hang
  EntityId b = world.createEntity();
  world.placeEntity(b, 7, 7);
  EXPECT_EQ(world.topObjectAt(7, 7), b);
  EXPECT_EQ(world.topObjectAt(8, 0), NULL_ENTITY);
}

TEST(ECSWorldTest, ForEachAliveVisitsExactlyTheLiveEntities) {
  ECSWorld world(testConfig());
  EntityId a = world.createEntity();
  EntityId b = world.createEntity();
  world.createEntity();
  world.destroyEntity(b);
  std::vector<EntityId> seen;
  world.forEachAlive([&](EntityId e) { seen.push_back(e); });
  EXPECT_EQ(seen.size(), 2u);
  EXPECT_EQ(seen[0], a);
}

TEST(ECSWorldTest, HashIsDeterministicForSameState) {
  ECSWorld a(testConfig());
  ECSWorld b(testConfig());
  EntityId ea = a.createEntity();
  EntityId eb = b.createEntity();
  a.x(ea) = 1; b.x(eb) = 1;
  a.setTick(10); b.setTick(10);
  EXPECT_EQ(a.computeHash(), b.computeHash());
  a.setTick(11);
  EXPECT_NE(a.computeHash(), b.computeHash());
}
