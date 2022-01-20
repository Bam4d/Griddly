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
  m.attr("version") = "1.2.25";

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
  gdy.def("get_action_names", &Py_GDYWrapper::getExternalActionNames);
  gdy.def("get_action_input_mappings", &Py_GDYWrapper::getActionInputMappings);
  gdy.def("get_avatar_object", &Py_GDYWrapper::getAvatarObject);
  gdy.def("create_game", &Py_GDYWrapper::createGame);
  gdy.def("get_level_count", &Py_GDYWrapper::getLevelCount);
  

  py::class_<Py_GameWrapper, std::shared_ptr<Py_GameWrapper>> game_process(m, "GameProcess");
  
  // Register a player to the game
  game_process.def("register_player", &Py_GameWrapper::registerPlayer);
  
  // Initialize the game or reset the game state
  game_process.def("init", &Py_GameWrapper::init);
  game_process.def("reset", &Py_GameWrapper::reset);

  // Generic step function for multiple players and multiple actions per step
  game_process.def("step_parallel", &Py_GameWrapper::stepParallel);

  // Set the current map of the game (should be followed by reset or init)
  game_process.def("load_level", &Py_GameWrapper::loadLevel);
  game_process.def("load_level_string", &Py_GameWrapper::loadLevelString);
  
  // Get available actions for objects in the current game
  game_process.def("get_available_actions", &Py_GameWrapper::getAvailableActionNames);
  game_process.def("get_available_action_ids", &Py_GameWrapper::getAvailableActionIds);
  game_process.def("build_valid_action_trees", &Py_GameWrapper::buildValidActionTrees);

  // Width and height of the game grid 
  game_process.def("get_width", &Py_GameWrapper::getWidth);
  game_process.def("get_height", &Py_GameWrapper::getHeight);

  // Observation shapes
  game_process.def("get_global_observation_shape", &Py_GameWrapper::getGlobalObservationShape);
  game_process.def("get_player_observation_shape", &Py_GameWrapper::getPlayerObservationShape);

  // Tile size of the global observer
  game_process.def("get_tile_size", &Py_GameWrapper::getTileSize);
  game_process.def("observe", &Py_GameWrapper::observe);
  
  // Enable the history collection mode 
  game_process.def("enable_history", &Py_GameWrapper::enableHistory);

  // Create a copy of the game in its current state
  game_process.def("clone", &Py_GameWrapper::clone);

  // Get a dictionary containing the objects in the environment and their variable values
  game_process.def("get_state", &Py_GameWrapper::getState);

  // Get a specific variable value
  game_process.def("get_global_variable", &Py_GameWrapper::getGlobalVariables);

  // Get list of possible object names, ordered by ID
  game_process.def("get_object_names", &Py_GameWrapper::getObjectNames);

  // Get list of possible variable names, ordered by ID
  game_process.def("get_object_variable_names", &Py_GameWrapper::getObjectVariableNames);

  // Get a mapping of objects to their variable names
  game_process.def("get_object_variable_map", &Py_GameWrapper::getObjectVariableMap);

  // Get a list of the global variable names
  game_process.def("get_global_variable_names", &Py_GameWrapper::getGlobalVariableNames);

  // Get a list of the events that have happened in the game up to this point
  game_process.def("get_history", &Py_GameWrapper::getHistory, py::arg("purge")=true);
  
  // Release resources for vulkan stuff
  game_process.def("release", &Py_GameWrapper::release);

  // Create an entity observer given a configuration of the entities and the custom variables that we want to view in the features
  game_process.def("get_entity_observer", &Py_GameWrapper::createEntityObserver, py::arg("config")=py::dict());

  py::class_<Py_EntityObserverWrapper, std::shared_ptr<Py_EntityObserverWrapper>> entityObserver(m, "EntityObserver");
  entityObserver.def("observe", &Py_EntityObserverWrapper::observe);

  py::class_<Py_StepPlayerWrapper, std::shared_ptr<Py_StepPlayerWrapper>> player(m, "Player");
  player.def("step", &Py_StepPlayerWrapper::stepSingle);
  player.def("step_multi", &Py_StepPlayerWrapper::stepMulti);
  player.def("observe", &Py_StepPlayerWrapper::observe);
  player.def("get_tile_size", &Py_StepPlayerWrapper::getTileSize);


  py::enum_<ObserverType> observer_type(m, "ObserverType");
  observer_type.value("SPRITE_2D", ObserverType::SPRITE_2D);
  observer_type.value("BLOCK_2D", ObserverType::BLOCK_2D);
  observer_type.value("ISOMETRIC", ObserverType::ISOMETRIC);
  observer_type.value("VECTOR", ObserverType::VECTOR);
  observer_type.value("ASCII", ObserverType::ASCII);
  observer_type.value("NONE", ObserverType::NONE);

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
