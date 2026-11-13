// FogOfWar / Camera / CameraFrame / HealthBars observer options, rendered by the engine from GDY config
// (tests/resources/fog_of_war.yaml: 12x6 map, player 1 unit at (1,1), player 2 unit at (10,4), a
// neutral rock at (7,3); units see 2 cells; tiles are 10px).
#include "gtest/gtest.h"

#include <memory>
#include <set>
#include <string>

#include "Griddly/Core/GDY/Actions/Action.hpp"
#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/Observers/EntityObserver.hpp"
#include "Griddly/Core/Observers/SDLGridObserver.hpp"
#include "Griddly/Core/Players/Player.hpp"
#include "Griddly/Core/TurnBasedGameProcess.hpp"

namespace griddly {

namespace {

const uint32_t kTile = 10;

struct FogGame {
  std::shared_ptr<GDYFactory> gdy;
  std::shared_ptr<TurnBasedGameProcess> process;
  std::shared_ptr<Player> p1;
  std::shared_ptr<Player> p2;

  explicit FogGame(const std::string& playerObserver, const std::string& globalObserver = "MiniMap") {
    ResourceConfig resourceConfig{"resources/games", "resources/images", ""};
    gdy = std::make_shared<GDYFactory>(std::make_shared<ObjectGenerator>(), std::make_shared<TerminationGenerator>(), resourceConfig);
    gdy->initializeFromFile("tests/resources/fog_of_war.yaml");
    process = std::make_shared<TurnBasedGameProcess>(globalObserver, gdy, std::make_shared<Grid>());
    p1 = std::make_shared<Player>(1, "p1", playerObserver, process);
    p2 = std::make_shared<Player>(2, "p2", playerObserver, process);
    process->addPlayer(p1);
    process->addPlayer(p2);
    process->init(false);
    process->reset();
  }

  // an action of `player`'s unit at `from`, with input `id` of `name`
  void act(const std::shared_ptr<Player>& player, glm::ivec2 from, const std::string& name, uint32_t id) {
    // findActionInputsDefinition returns a copy: keep the mapping by value
    const auto mapping = gdy->findActionInputsDefinition(name).inputMappings.at(id);
    auto action = std::make_shared<Action>(process->getGrid(), name, player->getId(), 0, mapping.metaData);
    action->init(from, from + mapping.vectorToDest);
    player->performActions({action}, true);
  }

  std::shared_ptr<SDLGridObserver> observer(const std::shared_ptr<Player>& player) const {
    return std::dynamic_pointer_cast<SDLGridObserver>(player->getObserver());
  }
};

struct Image {
  const uint8_t* data;
  uint32_t width;
  uint32_t height;

  // RGBA of the pixel at the centre of cell (x, y) (+ offset in pixels)
  const uint8_t* cell(int32_t x, int32_t y, int32_t ox = 0, int32_t oy = 0) const {
    const auto px = static_cast<uint32_t>(x * kTile + kTile / 2 + ox);
    const auto py = static_cast<uint32_t>(y * kTile + kTile / 2 + oy);
    return data + (static_cast<size_t>(py) * width + px) * 4;
  }
  int brightness(int32_t x, int32_t y, int32_t ox = 0, int32_t oy = 0) const {
    const auto* p = cell(x, y, ox, oy);
    return p[0] + p[1] + p[2];
  }
};

glm::ivec2 unitOfPlayer(FogGame& game, uint32_t playerId) {
  for (const auto& o : game.process->getGrid()->getObjects()) {
    if (o->getObjectName() == "unit" && o->getPlayerId() == playerId) return o->getLocation();
  }
  return {-1, -1};
}

Image render(const std::shared_ptr<Observer>& observer) {
  auto tensor = std::dynamic_pointer_cast<TensorObservationInterface>(observer);
  const uint8_t* data = &tensor->update();
  const auto& shape = tensor->getShape();
  return {data, shape[1], shape[2]};
}

}  // namespace

TEST(FogOfWarTest, player_sees_only_cells_near_its_own_objects) {
  FogGame game("Map");
  auto image = render(game.p1->getObserver());
  ASSERT_EQ(image.width, 12 * kTile);
  ASSERT_EQ(image.height, 6 * kTile);

  // around its own unit the map is drawn; far away it is black (never seen)
  ASSERT_GT(image.brightness(1, 1), 60);
  // empty cells in sight show the (black) clear colour too, so compare against the fog state below
  ASSERT_EQ(image.brightness(8, 1), 0);
  // the enemy unit and the rock are not drawn under the fog
  ASSERT_EQ(image.brightness(10, 4), 0);
  ASSERT_EQ(image.brightness(7, 3), 0);

  const auto& fog = game.observer(game.p1)->fogState();
  ASSERT_EQ(fog.size(), 12u * 6u);
  ASSERT_EQ(fog[1 * 12 + 1], 2);
  ASSERT_EQ(fog[1 * 12 + 3], 2);   // 2 cells away
  ASSERT_EQ(fog[1 * 12 + 4], 0);   // 3 cells away
  ASSERT_EQ(fog[3 * 12 + 3], 0);   // corner of the 2-cell box is outside the disc

  // player 2 sees its own corner instead (fog is per player)
  auto image2 = render(game.p2->getObserver());
  ASSERT_GT(image2.brightness(10, 4), 60);
  ASSERT_EQ(image2.brightness(1, 1), 0);
}

TEST(FogOfWarTest, explored_cells_keep_terrain_but_hide_enemies) {
  FogGame game("Map");
  // walk player 1's unit right along row 1, then down next to the rock and back
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  game.act(game.p1, {6, 1}, "move", 4);  // (6,2): rock at (7,3) is now visible
  auto seen = render(game.p1->getObserver());
  const int rockVisible = seen.brightness(7, 3);
  ASSERT_GT(rockVisible, 60);

  game.act(game.p1, {6, 2}, "move", 2);  // back to (6,1)
  for (int x = 6; x > 1; x--) game.act(game.p1, {x, 1}, "move", 1);  // back to (1,1)
  auto image = render(game.p1->getObserver());
  const auto& fog = game.observer(game.p1)->fogState();
  ASSERT_EQ(fog[3 * 12 + 7], 1);  // explored, not visible
  // the neutral rock is still drawn, dimmed by the explored fog
  const int rockExplored = image.brightness(7, 3);
  ASSERT_GT(rockExplored, 0);
  ASSERT_LT(rockExplored, rockVisible);

  // the enemy unit is never drawn outside sight, even on explored cells: move it into the explored
  // area and check its cell looks like the empty explored cell next to it
  game.act(game.p2, {10, 4}, "move", 2);  // (10,3)
  game.act(game.p2, {10, 3}, "move", 1);  // (9,3)
  game.act(game.p2, {9, 3}, "move", 2);   // (9,2)
  game.act(game.p2, {9, 2}, "move", 1);   // (8,2): explored by player 1 earlier
  image = render(game.p1->getObserver());
  ASSERT_EQ(fog[2 * 12 + 8], 1);
  ASSERT_NEAR(image.brightness(8, 2), image.brightness(8, 1), 30);
}

// Pixel-exact: an enemy on an explored (uncovered) cell draws nothing at all - no sprite, no
// ownership shadow spilling into the neighbouring cells, no health bar - in Sprite2D and Block2D.
TEST(FogOfWarTest, enemies_on_explored_cells_draw_nothing) {
  FogGame game("Map");
  std::vector<std::shared_ptr<Observer>> views = {game.process->getPlayerObserver(1, "MapShadow"),
                                                   game.process->getPlayerObserver(1, "BlockFog")};
  // player 1 explores towards the rock and comes home: cells around (8,2) are explored, not visible
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  game.act(game.p1, {6, 1}, "move", 4);  // (6,2)
  game.act(game.p1, {6, 2}, "move", 2);
  for (int x = 6; x > 1; x--) game.act(game.p1, {x, 1}, "move", 1);
  const auto& visibility = game.process->getPlayerVisibility(1);
  for (int y = 1; y <= 3; y++) {
    for (int x = 7; x <= 9; x++) ASSERT_NE(visibility[y * 12 + x], VISIBLE) << x << "," << y;
  }
  ASSERT_EQ(visibility[2 * 12 + 8], EXPLORED);

  // the region around (8,2) with no enemy anywhere near
  auto region = [](const Image& image) {
    std::vector<uint8_t> out;
    for (uint32_t y = 1 * kTile; y < 4 * kTile; y++) {
      for (uint32_t x = 7 * kTile; x < 10 * kTile; x++) {
        const auto* p = image.data + (static_cast<size_t>(y) * image.width + x) * 4;
        out.insert(out.end(), p, p + 3);
      }
    }
    return out;
  };
  std::vector<std::vector<uint8_t>> before;
  for (auto& view : views) before.push_back(region(render(view)));

  // a damaged enemy (health bar) walks onto the explored cell (8,2)
  game.act(game.p2, {10, 4}, "hurt", 1);
  game.act(game.p2, {10, 4}, "move", 2);  // (10,3)
  game.act(game.p2, {10, 3}, "move", 1);  // (9,3)
  game.act(game.p2, {9, 3}, "move", 2);   // (9,2)
  game.act(game.p2, {9, 2}, "move", 1);   // (8,2)
  ASSERT_EQ(unitOfPlayer(game, 2), glm::ivec2(8, 2));
  ASSERT_EQ(game.process->getPlayerVisibility(1)[2 * 12 + 8], EXPLORED);

  for (size_t i = 0; i < views.size(); i++) {
    ASSERT_EQ(region(render(views[i])), before[i]) << "observer " << i << " shows the hidden enemy";
  }

  // sanity check: the same renderers do draw the enemy once player 1 can see it
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);  // (6,1): (8,2) in sight
  ASSERT_EQ(game.process->getPlayerVisibility(1)[2 * 12 + 8], VISIBLE);
  for (size_t i = 0; i < views.size(); i++) {
    ASSERT_NE(region(render(views[i])), before[i]) << "observer " << i;
  }
}

// The non-picture observers (Vector, ASCII, Entity) give a player's agent nothing hidden either
TEST(FogOfWarTest, tensor_and_entity_observations_hide_enemies_out_of_sight) {
  FogGame game("Map");
  auto vector = game.process->getPlayerObserver(1, "Vec");
  auto ascii = game.process->getPlayerObserver(1, "Text");
  auto entity = std::dynamic_pointer_cast<EntityObserver>(game.process->getPlayerObserver(1, "Ent"));
  auto vectorTensor = std::dynamic_pointer_cast<TensorObservationInterface>(vector);
  auto asciiTensor = std::dynamic_pointer_cast<TensorObservationInterface>(ascii);
  const auto unitChannel = game.process->getGrid()->getObjectIds().at("unit");

  auto vectorHasUnitAt = [&](int32_t x, int32_t y) {
    const uint8_t* data = &vectorTensor->update();
    const auto& strides = vectorTensor->getStrides();
    return data[x * strides[1] + y * strides[2] + unitChannel] != 0;
  };
  auto asciiAt = [&](int32_t x, int32_t y) {
    const uint8_t* data = &asciiTensor->update();
    const auto& strides = asciiTensor->getStrides();
    return static_cast<char>(data[x * strides[1] + y * strides[2]]);
  };
  auto entityUnits = [&]() {
    auto& observation = entity->update();
    return observation.observations.count("unit") ? observation.observations.at("unit").size() : 0u;
  };

  // at the start player 1 only knows its own unit; the enemy at (10,4) is out of sight
  ASSERT_TRUE(vectorHasUnitAt(1, 1));
  ASSERT_FALSE(vectorHasUnitAt(10, 4));
  ASSERT_EQ(asciiAt(1, 1), 'u');
  ASSERT_EQ(asciiAt(10, 4), '.');
  ASSERT_EQ(entityUnits(), 1u);
  ASSERT_EQ(asciiAt(7, 3), '.');  // the rock is not explored yet

  // explore past the rock and come back; the enemy then stands on an explored cell
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  game.act(game.p1, {6, 1}, "move", 4);
  game.act(game.p1, {6, 2}, "move", 2);
  for (int x = 6; x > 1; x--) game.act(game.p1, {x, 1}, "move", 1);
  game.act(game.p2, {10, 4}, "move", 2);
  game.act(game.p2, {10, 3}, "move", 1);
  game.act(game.p2, {9, 3}, "move", 2);
  game.act(game.p2, {9, 2}, "move", 1);  // (8,2): explored by player 1, not visible
  ASSERT_EQ(game.process->getPlayerVisibility(1)[2 * 12 + 8], EXPLORED);
  ASSERT_FALSE(vectorHasUnitAt(8, 2));
  ASSERT_EQ(asciiAt(8, 2), '.');
  ASSERT_EQ(entityUnits(), 1u);
  ASSERT_EQ(asciiAt(7, 3), 'r');  // explored terrain stays known

  // in sight: all three observations show it
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  ASSERT_TRUE(vectorHasUnitAt(8, 2));
  ASSERT_EQ(asciiAt(8, 2), 'u');
  ASSERT_EQ(entityUnits(), 2u);
}

TEST(FogOfWarTest, camera_follows_global_variables_and_clamps_to_the_map) {
  FogGame game("Cam");
  auto observer = game.observer(game.p1);
  auto image = render(game.p1->getObserver());
  // 4x3 cells, starting clamped at the top-left
  ASSERT_EQ(image.width, 4 * kTile);
  ASSERT_EQ(image.height, 3 * kTile);
  ASSERT_EQ(observer->cameraOrigin(), glm::ivec2(0, 0));
  // player 1's own unit (green) at (1,1)
  const auto* unit = image.cell(1, 1);
  ASSERT_GT(unit[1], 150);
  ASSERT_LT(unit[0], 80);

  // pan player 1's camera to (5,2): the window is centred there -> origin (3,1)
  for (int i = 0; i < 5; i++) game.act(game.p1, {1, 1}, "pan", 1);
  for (int i = 0; i < 2; i++) game.act(game.p1, {1, 1}, "pan", 2);
  image = render(game.p1->getObserver());
  ASSERT_EQ(observer->cameraOrigin(), glm::ivec2(3, 1));
  // player 2's camera did not move
  render(game.p2->getObserver());
  ASSERT_EQ(game.observer(game.p2)->cameraOrigin(), glm::ivec2(0, 0));

  // far beyond the map edge: clamped so the window stays on the map (12x6 map, 4x3 window)
  for (int i = 0; i < 20; i++) game.act(game.p1, {1, 1}, "pan", 1);
  for (int i = 0; i < 20; i++) game.act(game.p1, {1, 1}, "pan", 2);
  render(game.p1->getObserver());
  ASSERT_EQ(observer->cameraOrigin(), glm::ivec2(8, 3));
}

TEST(FogOfWarTest, minimap_outlines_each_players_camera_window) {
  FogGame game("Map");
  // player 2 pans its camera to (9,4): a 4x3 window centred there starts at (7,3)
  for (int i = 0; i < 9; i++) game.act(game.p2, {10, 4}, "pan", 1);
  for (int i = 0; i < 4; i++) game.act(game.p2, {10, 4}, "pan", 2);
  auto image = render(game.process->getObserver());  // global observer: MiniMap
  ASSERT_EQ(image.width, 12 * kTile);

  auto pixel = [&](uint32_t x, uint32_t y) { return image.data + (static_cast<size_t>(y) * image.width + x) * 4; };
  // player 1's window at the origin (cells 0..3 x 0..2): green outline on the top-left edge
  ASSERT_GT(pixel(1, 15)[1], 200);
  ASSERT_LT(pixel(1, 15)[0], 60);
  // player 2's window (cells 7..10 x 3..5): red outline on its left edge
  ASSERT_GT(pixel(7 * kTile + 1, 4 * kTile)[0], 200);
  ASSERT_LT(pixel(7 * kTile + 1, 4 * kTile)[1], 60);
  // inside a window (no object) stays the background
  ASSERT_LT(pixel(2 * kTile + 5, 2 * kTile + 5)[1], 60);
}

TEST(FogOfWarTest, health_bars_over_damaged_objects) {
  FogGame game("Map");
  auto before = render(game.p1->getObserver());
  // full health: no bar (the top row of the unit's cell is the sprite / background)
  const int topBefore = before.brightness(1, 1, 0, -4);

  game.act(game.p1, {1, 1}, "hurt", 1);  // 10 -> 5 health
  auto after = render(game.p1->getObserver());
  // a bar at the top of the cell: the left part is coloured (yellow at 50%), the right part dark
  const auto* left = after.cell(1, 1, -3, -4);
  ASSERT_GT(left[0], 200);
  ASSERT_GT(left[1], 150);
  ASSERT_LT(left[2], 80);
  const auto* right = after.cell(1, 1, 2, -4);
  ASSERT_LT(right[0] + right[1] + right[2], 120);
  ASSERT_NE(topBefore, after.brightness(1, 1, -3, -4));
}

TEST(FogOfWarTest, fog_resets_with_the_episode) {
  FogGame game("Map");
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  render(game.p1->getObserver());
  ASSERT_EQ(game.observer(game.p1)->fogState()[1 * 12 + 7], 2);

  game.process->reset();
  render(game.p1->getObserver());
  ASSERT_EQ(game.observer(game.p1)->fogState()[1 * 12 + 7], 0);
}

TEST(FogOfWarTest, extra_player_observers_render_from_that_players_view) {
  // the game's players use the camera; each also gets a fogged full-map view ("Map") on demand
  FogGame game("Cam", "MiniMap");
  auto map1 = game.process->getPlayerObserver(1, "Map");
  auto map2 = game.process->getPlayerObserver(2, "Map");
  ASSERT_EQ(map1, game.process->getPlayerObserver(1, "Map"));  // created once
  auto image1 = render(map1);
  auto image2 = render(map2);
  ASSERT_EQ(image1.width, 12 * kTile);
  ASSERT_GT(image1.brightness(1, 1), 60);
  ASSERT_EQ(image1.brightness(10, 4), 0);
  ASSERT_GT(image2.brightness(10, 4), 60);
  ASSERT_EQ(image2.brightness(1, 1), 0);

  // reset with the game: explored cells are forgotten
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  render(map1);
  ASSERT_EQ(std::dynamic_pointer_cast<SDLGridObserver>(map1)->fogState()[1 * 12 + 7], 2);
  game.process->reset();
  render(map1);
  ASSERT_EQ(std::dynamic_pointer_cast<SDLGridObserver>(map1)->fogState()[1 * 12 + 7], 0);

  ASSERT_THROW(game.process->getPlayerObserver(3, "Map"), std::invalid_argument);
}

// The player state is filtered by the same fog: what a player (AI or UI) may know
std::set<std::string> objectsIn(const GameState& state, const GameStateMapping& mapping) {
  std::set<std::string> out;
  for (const auto& object : state.objectData) {
    const auto location = object.getLocation(object.getVariableIndexes(mapping));
    out.insert(object.name + "@" + std::to_string(location.x) + "," + std::to_string(location.y) + "/p" +
               std::to_string(object.variables[GameStateMapping::playerIdIdx]));
  }
  return out;
}

TEST(FogOfWarTest, player_state_only_holds_what_the_player_can_see) {
  FogGame game("Map");
  const auto& mapping = game.process->getGameStateMapping();

  // the full state has both units and the rock; player 1 sees only its own unit, player 2 its own
  auto full = objectsIn(game.process->getGameState(), mapping);
  ASSERT_TRUE(full.count("unit@10,4/p2"));
  ASSERT_TRUE(full.count("rock@7,3/p0"));
  auto p1 = objectsIn(game.process->getPlayerGameState(1), mapping);
  ASSERT_TRUE(p1.count("unit@1,1/p1"));
  ASSERT_FALSE(p1.count("unit@10,4/p2"));
  ASSERT_FALSE(p1.count("rock@7,3/p0"));
  auto p2 = objectsIn(game.process->getPlayerGameState(2), mapping);
  ASSERT_TRUE(p2.count("unit@10,4/p2"));
  ASSERT_FALSE(p2.count("unit@1,1/p1"));

  // per-player variables: only the player's own slot is filled in
  game.act(game.p1, {1, 1}, "pan", 1);  // player 1: cam_x 0 -> 1
  const auto camIdx = mapping.globalVariableNameToIdx.at("cam_x");
  const auto secretIdx = mapping.globalVariableNameToIdx.at("secret");
  auto s1 = game.process->getPlayerGameState(1);
  ASSERT_EQ(s1.globalData[camIdx][1], 1);
  ASSERT_EQ(s1.globalData[secretIdx][1], 7);
  ASSERT_EQ(s1.globalData[secretIdx][2], 0);  // player 2's value is hidden
  auto s2 = game.process->getPlayerGameState(2);
  ASSERT_EQ(s2.globalData[secretIdx][2], 7);
  ASSERT_EQ(s2.globalData[secretIdx][1], 0);

  // walk next to the rock: it becomes known, and stays known (explored) after walking away;
  // the enemy unit is known only while in sight
  for (int x = 1; x < 6; x++) game.act(game.p1, {x, 1}, "move", 3);
  game.act(game.p1, {6, 1}, "move", 4);  // (6,2)
  ASSERT_TRUE(objectsIn(game.process->getPlayerGameState(1), mapping).count("rock@7,3/p0"));
  game.act(game.p2, {10, 4}, "move", 1);  // (9,4): distance 3 from (6,2)... sight 2 -> hidden
  ASSERT_FALSE(objectsIn(game.process->getPlayerGameState(1), mapping).count("unit@9,4/p2"));
  game.act(game.p2, {9, 4}, "move", 2);  // (9,3)
  game.act(game.p2, {9, 3}, "move", 1);  // (8,3): dx 2, dy 1 -> inside the disc
  ASSERT_TRUE(objectsIn(game.process->getPlayerGameState(1), mapping).count("unit@8,3/p2"));
  game.act(game.p1, {6, 2}, "move", 2);  // (6,1)
  for (int x = 6; x > 1; x--) game.act(game.p1, {x, 1}, "move", 1);  // back home
  auto later = objectsIn(game.process->getPlayerGameState(1), mapping);
  ASSERT_TRUE(later.count("rock@7,3/p0"));
  ASSERT_FALSE(later.count("unit@8,3/p2"));

  // the observers show the same fog as the state
  render(game.p1->getObserver());
  ASSERT_EQ(game.observer(game.p1)->fogState(), game.process->getPlayerVisibility(1));
}

// ----- game clock: actions recorded between ticks run together on the tick -----

std::shared_ptr<Action> makeAction(FogGame& game, uint32_t playerId, glm::ivec2 from, const std::string& name, uint32_t id) {
  const auto mapping = game.gdy->findActionInputsDefinition(name).inputMappings.at(id);
  auto action = std::make_shared<Action>(game.process->getGrid(), name, playerId, 0, mapping.metaData);
  action->init(from, from + mapping.vectorToDest);
  return action;
}

glm::ivec2 unitOf(FogGame& game, uint32_t playerId) {
  for (const auto& o : game.process->getGrid()->getObjects()) {
    if (o->getObjectName() == "unit" && o->getPlayerId() == playerId) return o->getLocation();
  }
  return {-1, -1};
}

TEST(FogOfWarTest, queued_actions_run_together_on_the_tick) {
  FogGame game("Map");
  auto& process = *game.process;
  const auto t0 = *process.getGrid()->getTickCount();

  // record both players' moves: nothing happens until the clock ticks
  process.queueActions(1, {makeAction(game, 1, {1, 1}, "move", 3)});
  process.queueActions(2, {makeAction(game, 2, {10, 4}, "move", 1)});
  ASSERT_EQ(process.getQueuedActionCount(), 2u);
  ASSERT_EQ(process.getQueuedActionCount(1), 1u);
  ASSERT_EQ(unitOf(game, 1), glm::ivec2(1, 1));
  ASSERT_EQ(*process.getGrid()->getTickCount(), t0);

  auto result = process.tick();
  ASSERT_EQ(result.ticks, 1u);
  ASSERT_EQ(result.actions, 2u);
  // both moves ran, in the same single tick, and the queue is empty again
  ASSERT_EQ(unitOf(game, 1), glm::ivec2(2, 1));
  ASSERT_EQ(unitOf(game, 2), glm::ivec2(9, 4));
  ASSERT_EQ(*process.getGrid()->getTickCount(), t0 + 1);
  ASSERT_EQ(process.getQueuedActionCount(), 0u);

  // an empty tick still advances time
  process.tick();
  ASSERT_EQ(*process.getGrid()->getTickCount(), t0 + 2);

  // clearing drops recorded actions
  process.queueActions(1, {makeAction(game, 1, {2, 1}, "move", 3)});
  process.clearQueuedActions(1);
  process.tick();
  ASSERT_EQ(unitOf(game, 1), glm::ivec2(2, 1));
}

TEST(FogOfWarTest, the_clock_runs_ticks_at_the_gdy_tick_rate) {
  FogGame game("Map");
  auto& process = *game.process;
  ASSERT_DOUBLE_EQ(process.getTickRate(), 4.0);  // Environment.TickRate
  const auto t0 = *process.getGrid()->getTickCount();

  // 0.2s at 4 ticks/s is not a whole tick yet; 0.1s more makes 1.2 ticks -> one tick runs
  ASSERT_EQ(process.advanceTime(0.2).ticks, 0u);
  process.queueActions(1, {makeAction(game, 1, {1, 1}, "move", 3)});
  ASSERT_EQ(process.advanceTime(0.1).ticks, 1u);
  ASSERT_EQ(unitOf(game, 1), glm::ivec2(2, 1));
  // 1s -> 4 more ticks (0.2 carried over makes it 4.2) when allowed to catch up in one call
  ASSERT_EQ(process.advanceTime(1.0, 8).ticks, 4u);
  ASSERT_EQ(*process.getGrid()->getTickCount(), t0 + 5);
  // by default one tick per call, so players can record actions between ticks; the rest stays due
  ASSERT_EQ(process.advanceTime(0.5).ticks, 1u);
  ASSERT_EQ(process.advanceTime(0.0).ticks, 1u);
  ASSERT_EQ(process.advanceTime(0.0).ticks, 0u);
  ASSERT_EQ(*process.getGrid()->getTickCount(), t0 + 7);

  // paused: time passes but no ticks run, recorded actions wait
  process.setPaused(true);
  process.queueActions(1, {makeAction(game, 1, {2, 1}, "move", 3)});
  ASSERT_EQ(process.advanceTime(5.0).ticks, 0u);
  ASSERT_EQ(unitOf(game, 1), glm::ivec2(2, 1));
  // a manual tick still works while paused (step through the game)
  process.tick();
  ASSERT_EQ(unitOf(game, 1), glm::ivec2(3, 1));
  process.setPaused(false);

  // faster clock
  process.setTickRate(10.0);
  ASSERT_EQ(process.advanceTime(0.5, 8).ticks, 5u);
  // a long stall does not fast-forward through the backlog: at most 2 ticks stay due
  process.advanceTime(60.0, 1);
  ASSERT_LE(process.advanceTime(0.0, 100).ticks, 2u);
  // tick rate 0: no clock
  process.setTickRate(0.0);
  ASSERT_EQ(process.advanceTime(10.0).ticks, 0u);
}

TEST(FogOfWarTest, camera_without_variables_is_a_parse_error) {
  ResourceConfig resourceConfig{"resources/games", "resources/images", ""};
  auto gdy = std::make_shared<GDYFactory>(std::make_shared<ObjectGenerator>(), std::make_shared<TerminationGenerator>(), resourceConfig);
  gdy->initializeFromFile("tests/resources/fog_of_war.yaml");
  // a valid config parses
  ASSERT_NO_THROW(gdy->createObserver(std::make_shared<Grid>(), "Cam", 2, 1));
  ASSERT_THROW(
      {
        auto broken = std::make_shared<GDYFactory>(std::make_shared<ObjectGenerator>(), std::make_shared<TerminationGenerator>(), resourceConfig);
        std::stringstream yaml(
            "Version: \"0.1\"\n"
            "Environment:\n"
            "  Name: Broken\n"
            "  Observers:\n"
            "    Cam:\n"
            "      Type: BLOCK_2D\n"
            "      Camera: { X: nope, Y: nope }\n"
            "  Levels:\n"
            "    - |\n"
            "      .\n"
            "Actions: []\n"
            "Objects: []\n");
        broken->parseFromStream(yaml);
        broken->createObserver(std::make_shared<Grid>(), "Cam", 1, 1);
      },
      std::exception);
}

}  // namespace griddly
