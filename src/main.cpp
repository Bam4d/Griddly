#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include "Griddy/Core/Grid.hpp"
#include "Griddy/Core/Actions/MoveAction.hpp"
#include "Griddy/Objects/Object.hpp"

int main(int, char**) {
  std::cout << "Hello, world!\n";

  spdlog::set_level(spdlog::level::debug);

  griddy::Object test = griddy::Object(5, 6);

  griddy::GridLocation location = test.getLocation();

  std::cout << location.x << std::endl;
  std::cout << location.y << std::endl;

  griddy::Grid grid = griddy::Grid(10, 10);

  std::vector<std::shared_ptr<griddy::Action>> actions = std::vector<std::shared_ptr<griddy::Action>>();
  std::shared_ptr<griddy::MoveAction> moveAction = std::shared_ptr<griddy::MoveAction>(new griddy::MoveAction(griddy::Direction::UP, location));

  actions.push_back(std::move(moveAction));

  grid.update(actions);
}
