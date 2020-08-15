#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <spdlog/spdlog.h>

#include "wrapper/GridWrapper.cpp"
#include "wrapper/NumpyWrapper.cpp"
#include "wrapper/GDYReaderWrapper.cpp"

namespace py = pybind11;

namespace griddly {

PYBIND11_MODULE(python_griddly, m) {
  m.doc() = "Griddly python bindings";
  m.attr("version") = "0.1.0";

#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  spdlog::debug("Python Griddly module loaded!");

  py::class_<Py_GDYReaderWrapper, std::shared_ptr<Py_GDYReaderWrapper>> gdy_reader(m, "GDYReader");
  gdy_reader.def(py::init<std::string, std::string>());
  gdy_reader.def("load", &Py_GDYReaderWrapper::loadGDYFile);
  gdy_reader.def("load_string", &Py_GDYReaderWrapper::loadGDYString);


  py::class_ <Py_GridWrapper, std::shared_ptr<Py_GridWrapper>> grid(m, "Grid");
  grid.def("set_tile_size", &Py_GridWrapper::setTileSize);
  grid.def("get_tile_size", &Py_GridWrapper::getTileSize);
  grid.def("set_max_steps", &Py_GridWrapper::setMaxSteps);
  grid.def("get_width", &Py_GridWrapper::getWidth);
  grid.def("get_height", &Py_GridWrapper::getHeight);
  grid.def("get_player_count", &Py_GridWrapper::getPlayerCount);
  grid.def("get_action_input_mappings", &Py_GridWrapper::getActionInputMappings);
  grid.def("get_avatar_object", &Py_GridWrapper::getAvatarObject);
  grid.def("create_level", &Py_GridWrapper::createLevel);
  grid.def("load_level", &Py_GridWrapper::loadLevel);
  grid.def("load_level_string", &Py_GridWrapper::loadLevelString);
  grid.def("create_game", &Py_GridWrapper::createGame);
  grid.def("enable_history", &Py_GridWrapper::enableHistory);
  grid.def("add_object", &Py_GridWrapper::addObject);

  py::class_<Py_StepPlayerWrapper, std::shared_ptr<Py_StepPlayerWrapper>> player(m, "Player");
  player.def("step", &Py_StepPlayerWrapper::step);
  player.def("observe", &Py_StepPlayerWrapper::observe);

  py::class_<Py_GameProcessWrapper, std::shared_ptr<Py_GameProcessWrapper>> game_process(m, "GameProcess");
  game_process.def("register_player", &Py_GameProcessWrapper::registerPlayer);
  game_process.def("init", &Py_GameProcessWrapper::init);
  game_process.def("reset", &Py_GameProcessWrapper::reset);
  game_process.def("observe", &Py_GameProcessWrapper::observe);
  game_process.def("release", &Py_GameProcessWrapper::release);

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
}  // namespace griddly
