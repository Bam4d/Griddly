#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <spdlog/spdlog.h>

#include "wrapper/GDY.cpp"
#include "wrapper/GDYLoader.cpp"
#include "wrapper/NumpyWrapper.cpp"

namespace py = pybind11;

namespace griddly {

PYBIND11_MODULE(python_griddly, m) {
  m.doc() = "Griddly python bindings";
  m.attr("version") = "1.6.7";

#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  spdlog::debug("Python Griddly module loaded!");

  py::class_<Py_GDYLoader, std::shared_ptr<Py_GDYLoader>> gdy_reader(m, "GDYLoader");
  gdy_reader.def(py::init<std::string, std::string, std::string>());
  gdy_reader.def("load", &Py_GDYLoader::loadGDYFile);
  gdy_reader.def("load_string", &Py_GDYLoader::loadGDYString);

  py::class_<Py_GDY, std::shared_ptr<Py_GDY>> gdy(m, "GDY");
  gdy.def("set_max_steps", &Py_GDY::setMaxSteps);
  gdy.def("get_player_count", &Py_GDY::getPlayerCount);
  gdy.def("get_action_names", &Py_GDY::getExternalActionNames);
  gdy.def("get_action_input_mappings", &Py_GDY::getActionInputMappings);
  gdy.def("get_avatar_object", &Py_GDY::getAvatarObject);
  gdy.def("create_game", &Py_GDY::createGame);
  gdy.def("get_level_count", &Py_GDY::getLevelCount);
  gdy.def("get_observer_type", &Py_GDY::getObserverType);

  py::class_<Py_GameProcess, std::shared_ptr<Py_GameProcess>> game_process(m, "GameProcess");

  // Register a player to the game
  game_process.def("register_player", &Py_GameProcess::registerPlayer);

  // Initialize the game or reset the game state
  game_process.def("init", &Py_GameProcess::init);
  game_process.def("reset", &Py_GameProcess::reset);

  // Generic step function for multiple players and multiple actions per step
  game_process.def("step_parallel", &Py_GameProcess::stepParallel);

  // Set the current map of the game (should be followed by reset or init)
  game_process.def("load_level", &Py_GameProcess::loadLevel);
  game_process.def("load_level_string", &Py_GameProcess::loadLevelString);

  // Get available actions for objects in the current game
  game_process.def("get_available_actions", &Py_GameProcess::getAvailableActionNames);
  game_process.def("get_available_action_ids", &Py_GameProcess::getAvailableActionIds);
  game_process.def("build_valid_action_trees", &Py_GameProcess::buildValidActionTrees);

  // Width and height of the game grid
  game_process.def("get_width", &Py_GameProcess::getWidth);
  game_process.def("get_height", &Py_GameProcess::getHeight);

  // Tile Size (only used in some observers)
  game_process.def("get_tile_size", &Py_GameProcess::getTileSize);

  // Observation shapes
  game_process.def("get_global_observation_description", &Py_GameProcess::getGlobalObservationDescription);

  // Tile size of the global observer
  game_process.def("observe", &Py_GameProcess::observe);

  // Enable the history collection mode
  game_process.def("enable_history", &Py_GameProcess::enableHistory);

  // Create a copy of the game in its current state
  game_process.def("clone", &Py_GameProcess::clone);

  // Get a dictionary containing the objects in the environment and their variable values
  game_process.def("get_state", &Py_GameProcess::getState);

  // Load the state from a state object
  game_process.def("load_state", &Py_GameProcess::loadState);

  // Get a specific variable value
  game_process.def("get_global_variable", &Py_GameProcess::getGlobalVariables);

  // Get list of possible object names, ordered by ID
  game_process.def("get_object_names", &Py_GameProcess::getObjectNames);

  // Get list of possible variable names, ordered by ID
  game_process.def("get_object_variable_names", &Py_GameProcess::getObjectVariableNames);

  // Get a mapping of objects to their variable names
  game_process.def("get_object_variable_map", &Py_GameProcess::getObjectVariableMap);

  // Get a list of the global variable names
  game_process.def("get_global_variable_names", &Py_GameProcess::getGlobalVariableNames);

  // Get a list of the events that have happened in the game up to this point
  game_process.def("get_history", &Py_GameProcess::getHistory, py::arg("purge") = true);

  // Release resources for vulkan stuff
  game_process.def("release", &Py_GameProcess::release);

  game_process.def("seed", &Py_GameProcess::seedRandomGenerator);

  py::class_<Py_Player, std::shared_ptr<Py_Player>> player(m, "Player");
  player.def("step", &Py_Player::stepSingle);
  player.def("step_multi", &Py_Player::stepMulti);
  player.def("observe", &Py_Player::observe);
  player.def("get_observation_description", &Py_Player::getObservationDescription);

  py::enum_<ObserverType> observer_type(m, "ObserverType");
  observer_type.value("SPRITE_2D", ObserverType::SPRITE_2D);
  observer_type.value("BLOCK_2D", ObserverType::BLOCK_2D);
  observer_type.value("ISOMETRIC", ObserverType::ISOMETRIC);
  observer_type.value("VECTOR", ObserverType::VECTOR);
  observer_type.value("ASCII", ObserverType::ASCII);
  observer_type.value("ENTITY", ObserverType::ENTITY);
  observer_type.value("NONE", ObserverType::NONE);

  py::class_<NumpyWrapper<uint8_t>, std::shared_ptr<NumpyWrapper<uint8_t>>>(m, "Observation", py::buffer_protocol())
      .def_buffer([](NumpyWrapper<uint8_t> &m) -> py::buffer_info {
        return py::buffer_info(
            &m.getData(),
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
