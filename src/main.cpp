#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include <chrono>
#include "Griddy/Core/Actions/Gather.hpp"
#include "Griddy/Core/Actions/Move.hpp"
#include "Griddy/Core/Grid.hpp"
#include "Griddy/Core/Objects/Object.hpp"
#include "Griddy/Core/Objects/Terrain/Resource.hpp"
#include "Griddy/Core/Objects/Units/Harvester.hpp"
#include "Griddy/Core/Observers/TileObserver.hpp"
#include "Griddy/Core/Observers/BlockObserver.hpp"
#include "Griddy/Core/Players/StepPlayer.hpp"
#include "Griddy/Core/TurnBasedGameProcess.hpp"

int main(int, char**) {
  spdlog::set_level(spdlog::level::info);

  int playerId = 0;

  int gridX = 10;
  int gridY = 10;
  int tileSize = 48;

  auto player = std::shared_ptr<griddy::StepPlayer>(new griddy::StepPlayer(playerId, std::string("Test Player")));

  auto players = std::vector<std::shared_ptr<griddy::Player>>();
  players.push_back(player);

  auto observer = std::shared_ptr<griddy::BlockObserver>(new griddy::BlockObserver(tileSize));
  //auto tileObserver = std::shared_ptr<griddy::TileObserver>(new griddy::TileObserver());

  std::shared_ptr<griddy::Grid> grid = std::shared_ptr<griddy::Grid>(new griddy::Grid(gridX, gridY));

  auto gameProcess = std::shared_ptr<griddy::TurnBasedGameProcess>(new griddy::TurnBasedGameProcess(players, observer, grid));

  std::shared_ptr<griddy::Harvester> harvester = std::shared_ptr<griddy::Harvester>(new griddy::Harvester(playerId));
  std::shared_ptr<griddy::Resource> testResource = std::shared_ptr<griddy::Resource>(new griddy::Resource(10));

  grid->initObject({0, 0}, harvester);
  grid->initObject({4, 5}, testResource);

  gameProcess->init();

  auto startTime = std::chrono::system_clock::now();

  int ticks = 50000;

  for (auto i = 0; i < ticks; i++) {
    auto observation = gameProcess->observe(0);
    //observer->print(std::move(observation), grid);

    auto actions = std::vector<std::shared_ptr<griddy::Action>>();
    auto direction = i % 2 == 0 ? griddy::Direction::UP : griddy::Direction::RIGHT;
    auto moveAction = std::shared_ptr<griddy::Move>(new griddy::Move(direction, harvester->getLocation()));

    actions.push_back(std::move(moveAction));

    gameProcess->performActions(playerId, actions);

    if(i % 100 == 0) { 
      auto endTime = std::chrono::system_clock::now();

      auto elapsedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

      spdlog::info("FPS: {}", i / (elapsedMillis / 1000.0f));
    }
  }

  auto actions = std::vector<std::shared_ptr<griddy::Action>>();
  auto gatherAction = std::shared_ptr<griddy::Gather>(new griddy::Gather(griddy::Direction::UP, {4, 4}));

  actions.push_back(std::move(gatherAction));

  gameProcess->performActions(playerId, actions);

  auto observation = gameProcess->observe(0);

  observer->print(std::move(observation), grid);
}
