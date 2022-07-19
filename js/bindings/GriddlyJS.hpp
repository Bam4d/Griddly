#include <emscripten/bind.h>
#include <emscripten/html5.h>

#include "../../src/Griddly/Core/Grid.hpp"
#include "GriddlyJSGDYWrapper.hpp"

namespace e = emscripten;

class GriddlyJS {
 public:
  GriddlyJS();
  std::shared_ptr<GriddlyJSGDYWrapper> loadString(std::string levelString);
  std::string getExceptionMessage(intptr_t exceptionPtr);
};

template <typename K, typename V>
e::class_<std::unordered_map<K, V>> register_unordered_map(const char* name) {
  typedef std::unordered_map<K, V> MapType;

  size_t (MapType::*size)() const = &MapType::size;
  return e::class_<MapType>(name)
      .template constructor<>()
      .function("size", size)
      .function("get", e::internal::MapAccess<MapType>::get)
      .function("set", e::internal::MapAccess<MapType>::set)
      .function("keys", e::internal::MapAccess<MapType>::keys);
}


EMSCRIPTEN_BINDINGS(GriddlyJS) {
  // Classes
  e::class_<GriddlyJS>("GriddlyJS")
      .constructor()
      .function("loadString", &GriddlyJS::loadString)
      .function("getExceptionMessage", &GriddlyJS::getExceptionMessage);


  e::class_<GriddlyJSGDYWrapper>("GriddlyJSGDYWrapper")
      .smart_ptr<std::shared_ptr<GriddlyJSGDYWrapper>>("GriddlyJSGDYWrapper")
      .function("setMaxSteps", &GriddlyJSGDYWrapper::setMaxSteps)
      .function("getPlayerCount", &GriddlyJSGDYWrapper::getPlayerCount)
      .function("getAvatarObject", &GriddlyJSGDYWrapper::getAvatarObject)
      .function("getExternalActionNames", &GriddlyJSGDYWrapper::getExternalActionNames)
      .function("getLevelCount", &GriddlyJSGDYWrapper::getLevelCount)
      .function("getObserverType", &GriddlyJSGDYWrapper::getObserverType)
      .function("getActionInputMappings", &GriddlyJSGDYWrapper::getActionInputMappings)
      .function("createGame", &GriddlyJSGDYWrapper::createGame);

  e::class_<GriddlyJSGameWrapper>("GriddlyJSGameWrapper")
      .smart_ptr<std::shared_ptr<GriddlyJSGameWrapper>>("GriddlyJSGameWrapper")
      .function("getActionTypeId", &GriddlyJSGameWrapper::getActionTypeId)
      .function("init", &GriddlyJSGameWrapper::init)
      .function("release", &GriddlyJSGameWrapper::release)
      .function("registerPlayer", &GriddlyJSGameWrapper::registerPlayer)
      .function("loadLevel", &GriddlyJSGameWrapper::loadLevel)
      .function("loadLevelString", &GriddlyJSGameWrapper::loadLevelString)
      .function("reset", &GriddlyJSGameWrapper::reset)
      .function("getGlobalObservationDescription", &GriddlyJSGameWrapper::getGlobalObservationDescription)
      .function("observe", &GriddlyJSGameWrapper::observe)
      .function("stepParallel", &GriddlyJSGameWrapper::stepParallel)
      .function("getWidth", &GriddlyJSGameWrapper::getWidth)
      .function("getHeight", &GriddlyJSGameWrapper::getHeight)
      .function("getState", &GriddlyJSGameWrapper::getState)
      .function("getGlobalVariableNames", &GriddlyJSGameWrapper::getGlobalVariableNames)
      .function("getObjectVariableMap", &GriddlyJSGameWrapper::getObjectVariableMap)
      .function("getGlobalVariables", &GriddlyJSGameWrapper::getGlobalVariables)
      .function("getObjectNames", &GriddlyJSGameWrapper::getObjectNames)
      .function("getObjectVariableNames", &GriddlyJSGameWrapper::getObjectVariableNames)
      .function("seedRandomGenerator", &GriddlyJSGameWrapper::seedRandomGenerator);

  e::class_<GriddlyJSPlayerWrapper>("GriddlyJSPlayerWrapper")
      .smart_ptr<std::shared_ptr<GriddlyJSPlayerWrapper>>("GriddlyJSPlayerWrapper")
      .function("getObservationDescription", &GriddlyJSPlayerWrapper::getObservationDescription)
      .function("observe", &GriddlyJSPlayerWrapper::observe);
  
  // Types
  e::value_object<glm::ivec2>("glm::ivec2")
      .field("x", &glm::ivec2::x)
      .field("y", &glm::ivec2::y);

  e::value_object<glm::vec2>("glm::vec2")
      .field("x", &glm::vec2::x)
      .field("y", &glm::vec2::y);

  e::register_vector<int32_t>("IntVector");
  e::register_vector<uint32_t>("UInt32Vector");
  e::register_vector<uint8_t>("UInt8Vector");
  e::register_vector<std::string>("StringVector");

  e::enum_<griddly::ObserverType>("ObserverType")
      .value("VECTOR", griddly::ObserverType::VECTOR)
      .value("ASCII", griddly::ObserverType::ASCII)
      .value("ENTITY", griddly::ObserverType::ENTITY)
      .value("NONE", griddly::ObserverType::NONE);
}
