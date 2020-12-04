#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <spdlog/spdlog.h>

#include "wrapper/GriddlyLoaderWrapper.cpp"
#include "wrapper/GDYWrapper.cpp"
#include "wrapper/NumpyWrapper.cpp"

namespace py = pybind11;

namespace griddly {

PYBIND11_MODULE(python_griddly, m) {
  m.doc() = "Griddly python bindings";
  m.attr("version") = "0.1.9";

#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  spdlog::debug("Python Griddly module loaded!");

  py::class_<Py_GriddlyLoaderWrapper, std::shared_ptr<Py_GriddlyLoaderWrapper>> gdy_reader(m, "GDYReader");
  gdy_reader.def(py::init<std::string, std::string>());
  gdy_reader.def("load", &Py_GriddlyLoaderWrapper::loadGDYFile);
  gdy_reader.def("load_string", &Py_GriddlyLoaderWrapper::loadGDYString);

  py::class_<Py_GDYWrapper, std::shared_ptr<Py_GDYWrapper>> gdy(m, "GDY");
  gdy.def("set_max_steps", &Py_GDYWrapper::setMaxSteps);
  gdy.def("get_player_count", &Py_GDYWrapper::getPlayerCount);
  gdy.def("get_action_input_mappings", &Py_GDYWrapper::getActionInputMappings);
  gdy.def("get_avatar_object", &Py_GDYWrapper::getAvatarObject);
  gdy.def("create_game", &Py_GDYWrapper::createGame);
  

  py::class_<Py_GameWrapper, std::shared_ptr<Py_GameWrapper>> game_process(m, "GameProcess");
  
  // Register a player to the game
  game_process.def("register_player", &Py_GameWrapper::registerPlayer);
  
  // Initialize the game or reset the game state
  game_process.def("init", &Py_GameWrapper::init);
  game_process.def("reset", &Py_GameWrapper::reset);

  // Set the current map of the game (should be followed by reset or init)
  game_process.def("load_level", &Py_GameWrapper::loadLevel);
  game_process.def("load_level_string", &Py_GameWrapper::loadLevelString);
  
  // Get available actions for objects in the current game
  game_process.def("get_available_actions", &Py_GameWrapper::getAvailableActionNames);
  game_process.def("get_available_action_ids", &Py_GameWrapper::getAvailableActionIds);
  
  // Width and height of the game grid 
  game_process.def("get_width", &Py_GameWrapper::getWidth);
  game_process.def("get_height", &Py_GameWrapper::getHeight);

  // Tile size of the global observer
  game_process.def("get_tile_size", &Py_GameWrapper::getTileSize);
  game_process.def("observe", &Py_GameWrapper::observe);
  
  // Enable the history collection mode 
  game_process.def("enable_history", &Py_GameWrapper::enableHistory);

  // Create a copy of the game in its current state
  game_process.def("clone", &Py_GameWrapper::clone);
  
  // Release resources for vulkan stuff
  game_process.def("release", &Py_GameWrapper::release);

  py::class_<Py_StepPlayerWrapper, std::shared_ptr<Py_StepPlayerWrapper>> player(m, "Player");
  player.def("step", &Py_StepPlayerWrapper::step);
  player.def("observe", &Py_StepPlayerWrapper::observe);
  player.def("get_tile_size", &Py_StepPlayerWrapper::getTileSize);


  py::enum_<ObserverType> observer_type(m, "ObserverType");
  observer_type.value("NONE", ObserverType::NONE);
  observer_type.value("SPRITE_2D", ObserverType::SPRITE_2D);
  observer_type.value("BLOCK_2D", ObserverType::BLOCK_2D);
  observer_type.value("ISOMETRIC", ObserverType::ISOMETRIC);
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
