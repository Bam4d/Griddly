#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <spdlog/spdlog.h>

#include "wrapper/GridWrapper.cpp"
#include "wrapper/NumpyWrapper.cpp"
#include "wrapper/MapReaderWrapper.cpp"

namespace py = pybind11;

namespace griddy {

PYBIND11_MODULE(griddy, m) {
  m.doc() = "GriddyRTS python bindings";
  m.attr("version") = "0.0.1";

#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  spdlog::debug("Python Griddy module loaded!");

  py::class_<Py_MapReaderWrapper, std::shared_ptr<Py_MapReaderWrapper>> map_reader(m, "MapReader");
  map_reader.def(py::init<>());
  map_reader.def("load_map_file", &Py_MapReaderWrapper::loadMapFile);
  map_reader.def("load_map_string", &Py_MapReaderWrapper::loadMapString);

  py::class_<Py_GridWrapper, std::shared_ptr<Py_GridWrapper>> grid(m, "Grid");
  grid.def(py::init<int, int>());
  grid.def("get_width", &Py_GridWrapper::getHeight);
  grid.def("get_height", &Py_GridWrapper::getWidth);
  grid.def("create_game", &Py_GridWrapper::createGame);
  grid.def("add_object", &Py_GridWrapper::addObject);

  py::enum_<ObjectType> object_type(m, "ObjectType");
  object_type.value("FIXED_WALL", ObjectType::FIXED_WALL);
  object_type.value("PUSHABLE_WALL", ObjectType::PUSHABLE_WALL);
  object_type.value("HARVESTER", ObjectType::HARVESTER);
  object_type.value("MINERALS", ObjectType::MINERALS);
  object_type.value("PUNCHER", ObjectType::PUNCHER);
  object_type.value("PUSHER", ObjectType::PUSHER);

  py::class_<Py_StepPlayerWrapper, std::shared_ptr<Py_StepPlayerWrapper>> player(m, "Player");
  player.def("step", &Py_StepPlayerWrapper::step);

  py::class_<Py_GameProcessWrapper, std::shared_ptr<Py_GameProcessWrapper>> game_process(m, "GameProcess");
  game_process.def("add_player", &Py_GameProcessWrapper::addPlayer);
  game_process.def("start_game", &Py_GameProcessWrapper::startGame);
  game_process.def("observe", &Py_GameProcessWrapper::observe);

  py::enum_<ActionType> action_type(m, "ActionType");
  action_type.value("MOVE", ActionType::MOVE);
  action_type.value("GATHER", ActionType::GATHER);
  action_type.value("PUNCH", ActionType::PUNCH);

  py::enum_<Direction> direction(m, "Direction");
  direction.value("UP", Direction::UP);
  direction.value("DOWN", Direction::DOWN);
  direction.value("LEFT", Direction::LEFT);
  direction.value("RIGHT", Direction::RIGHT);

  py::enum_<ObserverType> observer_type(m, "ObserverType");
  observer_type.value("NONE", ObserverType::NONE);
  observer_type.value("SPRITE_2D", ObserverType::SPRITE_2D);
  observer_type.value("BLOCK_2D", ObserverType::BLOCK_2D);
  observer_type.value("VECTOR", ObserverType::VECTOR);

  py::class_<NumpyWrapper<uint8_t>, std::shared_ptr<NumpyWrapper<uint8_t>>>(m, "Observation", py::buffer_protocol())
      .def_buffer([](NumpyWrapper<uint8_t> &m) -> py::buffer_info {
        return py::buffer_info(
            m.getData(),
            m.getScalarSize(),
            py::format_descriptor<uint8_t>::format(),
            m.getShape().size(),
            m.getShape(),
            m.getStrides());
      })
      .def("shape", &NumpyWrapper<uint8_t>::getShape)
      .def("strides", &NumpyWrapper<uint8_t>::getStrides);
}
}  // namespace griddy
