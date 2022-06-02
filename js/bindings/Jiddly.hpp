#include <emscripten/bind.h>
#include <emscripten/html5.h>

#include "../../src/Griddly/Core/Grid.hpp"
#include "JiddlyGDYWrapper.hpp"

namespace e = emscripten;

class Jiddly {
 public:
  Jiddly();
  std::shared_ptr<JiddlyGDYWrapper> loadString(std::string levelString);
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

EMSCRIPTEN_BINDINGS(Jiddly) {
  // Classes
  e::class_<Jiddly>("Jiddly")
      .constructor()
      .function("loadString", &Jiddly::loadString);

  e::class_<JiddlyGDYWrapper>("JiddlyGDYWrapper")
      .smart_ptr<std::shared_ptr<JiddlyGDYWrapper>>("JiddlyGDYWrapper")
      .function("setMaxSteps", &JiddlyGDYWrapper::setMaxSteps)
      .function("getPlayerCount", &JiddlyGDYWrapper::getPlayerCount)
      .function("getAvatarObject", &JiddlyGDYWrapper::getAvatarObject)
      .function("getExternalActionNames", &JiddlyGDYWrapper::getExternalActionNames)
      .function("getLevelCount", &JiddlyGDYWrapper::getLevelCount)
      .function("getObserverType", &JiddlyGDYWrapper::getObserverType)
      .function("getActionInputMappings", &JiddlyGDYWrapper::getActionInputMappings)
      .function("createGame", &JiddlyGDYWrapper::createGame);

  e::class_<JiddlyGameWrapper>("JiddlyGameWrapper")
      .smart_ptr<std::shared_ptr<JiddlyGameWrapper>>("JiddlyGameWrapper")
      .function("getActionTypeId", &JiddlyGameWrapper::getActionTypeId)
      .function("init", &JiddlyGameWrapper::init)
      .function("release", &JiddlyGameWrapper::release)
      .function("registerPlayer", &JiddlyGameWrapper::registerPlayer)
      .function("loadLevel", &JiddlyGameWrapper::loadLevel)
      .function("loadLevelString", &JiddlyGameWrapper::loadLevelString)
      .function("reset", &JiddlyGameWrapper::reset)
      .function("getGlobalObservationDescription", &JiddlyGameWrapper::getGlobalObservationDescription)
      .function("observe", &JiddlyGameWrapper::observe)
      .function("stepParallel", &JiddlyGameWrapper::stepParallel)
      .function("getWidth", &JiddlyGameWrapper::getWidth)
      .function("getHeight", &JiddlyGameWrapper::getHeight)
      .function("getState", &JiddlyGameWrapper::getState)
      .function("getGlobalVariableNames", &JiddlyGameWrapper::getGlobalVariableNames)
      .function("getObjectVariableMap", &JiddlyGameWrapper::getObjectVariableMap)
      .function("getGlobalVariables", &JiddlyGameWrapper::getGlobalVariables)
      .function("getObjectNames", &JiddlyGameWrapper::getObjectNames)
      .function("getObjectVariableNames", &JiddlyGameWrapper::getObjectVariableNames)
      .function("seedRandomGenerator", &JiddlyGameWrapper::seedRandomGenerator);

  e::class_<JiddlyPlayerWrapper>("JiddlyPlayerWrapper")
      .smart_ptr<std::shared_ptr<JiddlyPlayerWrapper>>("JiddlyPlayerWrapper")
      .function("getObservationDescription", &JiddlyPlayerWrapper::getObservationDescription)
      .function("observe", &JiddlyPlayerWrapper::observe);
  
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
