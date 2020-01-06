#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include "Griddy/Core/Grid.hpp"
#include "Griddy/Core/Actions/MoveAction.hpp"
#include "Griddy/Core/Objects/Object.hpp"
#include "Griddy/Core/Objects/Terrain/Resource.hpp"
#include "Griddy/Core/Observers/TileObserver.hpp"

int main(int, char**) {
  std::cout << "Hello, world!\n";

  spdlog::set_level(spdlog::level::debug);

  griddy::TileObserver tileObserver = griddy::TileObserver();

  std::shared_ptr<griddy::Resource> testResource1 = std::shared_ptr<griddy::Resource>(new griddy::Resource(10));
  std::shared_ptr<griddy::Resource> testResource2 = std::shared_ptr<griddy::Resource>(new griddy::Resource(10));
  std::shared_ptr<griddy::Resource> testResource3 = std::shared_ptr<griddy::Resource>(new griddy::Resource(10));

  std::shared_ptr<griddy::Grid> grid = std::shared_ptr<griddy::Grid>(new griddy::Grid(10, 10));

  grid->initObject({0,0}, std::move(testResource1));
  grid->initObject({5,5}, std::move(testResource2));
  grid->initObject({8,7}, std::move(testResource3));

  std::unique_ptr<uint8_t[]> observation = tileObserver.observe(grid);

  tileObserver.print(std::move(observation), grid);

  std::vector<std::shared_ptr<griddy::Action>> actions = std::vector<std::shared_ptr<griddy::Action>>();
  std::shared_ptr<griddy::MoveAction> moveAction = std::shared_ptr<griddy::MoveAction>(new griddy::MoveAction(griddy::Direction::UP, {0,0}));

  actions.push_back(std::move(moveAction));

  grid->update(actions);
}
