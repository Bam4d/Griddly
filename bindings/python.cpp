#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <spdlog/spdlog.h>

#include "wrapper/GriddlyLoaderWrapper.cpp"
#include "wrapper/GDYWrapper.cpp"
#include "wrapper/NumpyWrapper.cpp"
#include "wrapper/VectorizedWrapper.cpp"

namespace py = nanobind;

namespace griddly {

NB_MODULE(python_griddly, m) {
  m.doc() = "Griddly python bindings";
  m.attr("version") = "1.6.7";

#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  spdlog::debug("Python Griddly module loaded!");

  py::class_<Py_GriddlyLoaderWrapper> gdy_reader(m, "GDYReader");
  gdy_reader.def(py::init<std::string, std::string, std::string>());
  gdy_reader.def("load", &Py_GriddlyLoaderWrapper::loadGDYFile);
  gdy_reader.def("load_string", &Py_GriddlyLoaderWrapper::loadGDYString);

  py::class_<Py_GDYWrapper> gdy(m, "GDY");
  gdy.def("set_max_steps", &Py_GDYWrapper::setMaxSteps);
  gdy.def("get_player_count", &Py_GDYWrapper::getPlayerCount);
  gdy.def("get_action_names", &Py_GDYWrapper::getExternalActionNames);
  gdy.def("get_action_input_mappings", &Py_GDYWrapper::getActionInputMappings);
  gdy.def("get_avatar_object", &Py_GDYWrapper::getAvatarObject);
  gdy.def("create_game", &Py_GDYWrapper::createGame);
  gdy.def("create_vectorized", &Py_GDYWrapper::createVectorized,
          py::arg("num_envs"), py::arg("num_threads") = 0, py::arg("observer_name") = "Sprite2D",
          py::arg("render_backend") = "auto");
  gdy.def("get_level_count", &Py_GDYWrapper::getLevelCount);
  gdy.def("get_observer_type", &Py_GDYWrapper::getObserverType);

  py::class_<Py_VectorizedWrapper>(m, "VectorizedGame")
      .def("num_envs", &Py_VectorizedWrapper::numEnvs)
      .def("num_threads", &Py_VectorizedWrapper::numThreads)
      .def("load_level", &Py_VectorizedWrapper::loadLevel)
      .def("load_level_string", &Py_VectorizedWrapper::loadLevelString)
      .def("reset", &Py_VectorizedWrapper::reset)
      .def("reset_from_snapshot", &Py_VectorizedWrapper::resetFromSnapshotAll)
      .def("seed", &Py_VectorizedWrapper::seed)
      .def("step", &Py_VectorizedWrapper::step)
      .def("observe", &Py_VectorizedWrapper::observe, py::arg("parallel") = true, py::arg("device") = false)
      .def("render_backend", &Py_VectorizedWrapper::renderBackend)
      .def("frame_width", &Py_VectorizedWrapper::frameWidth)
      .def("frame_height", &Py_VectorizedWrapper::frameHeight);


  py::class_<Py_GameWrapper> game_process(m, "GameProcess");
  
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

  // Tile Size (only used in some observers)
  game_process.def("get_tile_size", &Py_GameWrapper::getTileSize);

  // Observation shapes
  game_process.def("get_global_observation_description", &Py_GameWrapper::getGlobalObservationDescription);

  // Tile size of the global observer
  game_process.def("observe", &Py_GameWrapper::observe);

  // A named observer from a player's perspective (fog of war, camera): observe_player(player_id, name)
  game_process.def("observe_player", &Py_GameWrapper::observePlayer);
  
  // Enable the history collection mode 
  game_process.def("enable_history", &Py_GameWrapper::enableHistory);

  // Create a copy of the game in its current state
  game_process.def("clone", &Py_GameWrapper::clone);

  // Get a dictionary containing the objects in the environment and their variable values
  game_process.def("get_state", &Py_GameWrapper::getState);

  // The state one player may know under Environment.Player.FogOfWar (fog-filtered), with "Visibility"
  game_process.def("get_player_state", &Py_GameWrapper::getPlayerState);

  // Load the state from a state object
  game_process.def("load_state", &Py_GameWrapper::loadState);

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

  game_process.def("seed", &Py_GameWrapper::seedRandomGenerator);

  // Game clock (Environment.TickRate): players queue actions, the clock runs them together each tick
  game_process.def("tick", &Py_GameWrapper::tick);
  game_process.def("advance_time", &Py_GameWrapper::advanceTime, py::arg("seconds"), py::arg("max_ticks") = 1);
  game_process.def("clear_queued_actions", &Py_GameWrapper::clearQueuedActions, py::arg("player_id") = 0);
  game_process.def("get_queued_action_count", &Py_GameWrapper::getQueuedActionCount, py::arg("player_id") = 0);
  game_process.def("set_tick_rate", &Py_GameWrapper::setTickRate);
  game_process.def("get_tick_rate", &Py_GameWrapper::getTickRate);
  game_process.def("set_paused", &Py_GameWrapper::setPaused);
  game_process.def("is_paused", &Py_GameWrapper::isPaused);


  py::class_<Py_StepPlayerWrapper> player(m, "Player");
  player.def("step", &Py_StepPlayerWrapper::stepSingle);
  player.def("step_multi", &Py_StepPlayerWrapper::stepMulti);
  // record actions for the next game-clock tick (see GameProcess.tick / advance_time)
  player.def("queue", &Py_StepPlayerWrapper::queueAction);
  player.def("queue_multi", &Py_StepPlayerWrapper::queueActions);
  player.def("observe", &Py_StepPlayerWrapper::observe);
  player.def("get_observation_description", &Py_StepPlayerWrapper::getObservationDescription);


  py::enum_<ObserverType> observer_type(m, "ObserverType");
  observer_type.value("SPRITE_2D", ObserverType::SPRITE_2D);
  observer_type.value("BLOCK_2D", ObserverType::BLOCK_2D);
  observer_type.value("ISOMETRIC", ObserverType::ISOMETRIC);
  observer_type.value("VECTOR", ObserverType::VECTOR);
  observer_type.value("ASCII", ObserverType::ASCII);
  observer_type.value("ENTITY", ObserverType::ENTITY);
  observer_type.value("NONE", ObserverType::NONE);

  py::class_<NumpyWrapper<uint8_t>>(m, "Observation")
      .def("to_numpy", [](NumpyWrapper<uint8_t> &m) {
        auto shape = m.getShape();
        auto strides = m.getStrides();
        std::vector<size_t> shapeN(shape.begin(), shape.end());
        std::vector<int64_t> strideN(strides.begin(), strides.end());
        // pass strides and keep the wrapper alive as owner: the buffer is
        // interleaved RGBA, the logical shape is (channels, width, height)
        return py::ndarray<uint8_t, py::ndim<3>, py::ro, py::numpy>(&m.getData(), shapeN.size(), shapeN.data(), py::handle(), strideN.data());
      })
      .def("__array__", [](NumpyWrapper<uint8_t> &m, py::handle, py::handle) {
        auto shape = m.getShape();
        auto strides = m.getStrides();
        std::vector<size_t> shapeN(shape.begin(), shape.end());
        std::vector<int64_t> strideN(strides.begin(), strides.end());
        return py::ndarray<uint8_t, py::ndim<3>, py::ro, py::numpy>(&m.getData(), shapeN.size(), shapeN.data(), py::handle(), strideN.data());
      }, py::arg("dtype") = py::none(), py::arg("copy") = py::none())
      // Zero-copy view of the raw interleaved RGBA buffer as (height, width, 4),
      // e.g. for piping straight to a video encoder without any numpy copy
      .def("to_image", [](NumpyWrapper<uint8_t> &m) {
        auto shape = m.getShape();    // {3, width, height}
        auto strides = m.getStrides(); // {1, 4, 4*width}
        if (shape.size() != 3 || strides.size() != 3) {
          throw std::runtime_error("to_image() needs a 3D image observation with strides");
        }
        std::vector<size_t> imgShape = {shape[2], shape[1], 4};
        std::vector<int64_t> imgStrides = {(int64_t)strides[2], (int64_t)strides[1], (int64_t)strides[0]};
        return py::ndarray<uint8_t, py::ndim<3>, py::ro, py::numpy>(&m.getData(), imgShape.size(), imgShape.data(), py::handle(), imgStrides.data());
      })
      .def("shape", &NumpyWrapper<uint8_t>::getShape)
      .def("strides", &NumpyWrapper<uint8_t>::getStrides);
}
}  // namespace griddly
