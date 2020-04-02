#include <pybind11/pybind11.h>

#include "wrapper/GridWrapper.cpp"
//#include "wrapper/StepPlayerWrapper.cpp"

// #include "../src/Griddy/Core/Grid.hpp"
// #include "../src/Griddy/Core/Observers/TileObserver.hpp"

namespace py = pybind11;

namespace griddy {

// std::shared_ptr<griddy::Observer> create_observer() {
//   return std::shared_ptr<griddy::Observer>(new griddy::TileObserver());
// }

// std::shared_ptr<griddy::GameProcess> create_game_process(
//     std::vector<std::shared_ptr<griddy::Player>> players,
//     std::shared_ptr<griddy::Observer> observer,
//     std::shared_ptr<griddy::Grid> grid) {
//   return std::shared_ptr<griddy::GameProcess>(new griddy::TurnBasedGameProcess(players, observer, grid));
// }

PYBIND11_MODULE(griddy, m) {
  m.doc() = "GriddyRTS python bindings";
  m.attr("version") = "0.0.1";


  py::class_<Py_GridWrapper, std::shared_ptr<Py_GridWrapper>> grid(m, "Grid");
  grid.def(py::init<int, int>());
  grid.def("get_width", &Py_GridWrapper::getHeight);
  grid.def("get_height", &Py_GridWrapper::getWidth);
  grid.def("add_player", &Py_GridWrapper::addPlayer);
  grid.def("start_game", &Py_GridWrapper::startGame);

  py::class_<Py_PlayerWrapper> player(m, "Player");
  player.def("step", &Py_PlayerWrapper::step);

  py::class_<Py_GameProcessWrapper> env(m, "Env");


  py::enum_<ActionType> action_type(m, "ActionType");
  action_type.value("MOVE", ActionType::MOVE);
  action_type.value("GATHER", ActionType::GATHER);
  action_type.value("PUNCH", ActionType::PUNCH);

  py::enum_<Direction> direction(m, "Direction");
  direction.value("UP", Direction::UP);
  direction.value("DOWN", Direction::DOWN);
  direction.value("LEFT", Direction::LEFT);
  direction.value("RIGHT", Direction::RIGHT);

  py::enum_<ObserverType> observer(m, "ObserverType");
  observer.value("BLOCK_2D", ObserverType::BLOCK_2D);
  observer.value("TILE", ObserverType::TILE);

  //env.def("observe". &Py_GameProcessWrapper::observe);

//   m.def("create_grid", &create_grid, "Create a new grid environment");

//   //Define observer types
//   m.def("create_observer", &create_observer, "Create a new observer");

//   //Define game process types
//   m.def("create_game_process", &create_game_process, "Create a new grid environment");
}
}  // namespace grippy
