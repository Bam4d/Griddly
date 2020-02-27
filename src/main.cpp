#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include "Griddy/Core/Grid.hpp"
#include "Griddy/Core/Actions/Gather.hpp"
#include "Griddy/Core/Actions/Move.hpp"
#include "Griddy/Core/Objects/Object.hpp"
#include "Griddy/Core/Objects/Terrain/Resource.hpp"
#include "Griddy/Core/Objects/Units/Harvester.hpp"
#include "Griddy/Core/Observers/TileObserver.hpp"
#include "Griddy/Core/Players/StepPlayer.hpp"
#include "Griddy/Core/TurnBasedGameProcess.hpp"

int main(int, char**) {
  std::cout << "Hello, world!\n";

  spdlog::set_level(spdlog::level::debug);

  int playerId = 0;

  auto player = std::shared_ptr<griddy::StepPlayer>(new griddy::StepPlayer(playerId, std::string("Test Player")));

  auto players = std::vector<std::shared_ptr<griddy::Player>>();
  players.push_back(player);

  auto tileObserver = std::shared_ptr<griddy::TileObserver>(new griddy::TileObserver());

  std::shared_ptr<griddy::Grid> grid = std::shared_ptr<griddy::Grid>(new griddy::Grid(10, 10));

  auto gameProcess = std::shared_ptr<griddy::TurnBasedGameProcess>(new griddy::TurnBasedGameProcess(players, tileObserver, grid));

  std::shared_ptr<griddy::Harvester> harvester = std::shared_ptr<griddy::Harvester>(new griddy::Harvester(playerId));
  std::shared_ptr<griddy::Resource> testResource = std::shared_ptr<griddy::Resource>(new griddy::Resource(10));
  
  grid->initObject({4,4}, std::move(harvester));
  grid->initObject({4,5}, std::move(testResource));

  std::unique_ptr<uint8_t[]> observation = gameProcess->observe(0);

  tileObserver->print(std::move(observation), grid);

  auto actions = std::vector<std::shared_ptr<griddy::Action>>();
  auto gatherAction = std::shared_ptr<griddy::Gather>(new griddy::Gather(griddy::Direction::UP, {4,4}));

  actions.push_back(std::move(gatherAction));

  gameProcess->performActions(playerId, actions);
}
