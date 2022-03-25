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

template<typename K, typename V>
e::class_<std::unordered_map<K, V>> register_unordered_map(const char* name) {
    typedef std::unordered_map<K,V> MapType;

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
      .function("getActionInputMappings", &JiddlyGDYWrapper::getActionInputMappings);

  // Types
  e::value_object<glm::ivec2>("glm::ivec2")
      .field("x", &glm::ivec2::x)
      .field("y", &glm::ivec2::y);
    
  e::value_object<glm::vec2>("glm::vec2")
      .field("x", &glm::vec2::x)
      .field("y", &glm::vec2::y);

  e::register_vector<int32_t>("IntVector");
  e::register_vector<uint32_t>("UIntVector");
  e::register_vector<std::string>("StringVector");

  register_unordered_map<uint32_t, griddly::ActionInputsDefinition>("map<uint32_t, griddly::ActionInputsDefinition>");
  e::value_object<griddly::ActionInputsDefinition>("ActionInputsDefinition")
      .field("inputMappings", &griddly::ActionInputsDefinition::inputMappings)
      .field("relative", &griddly::ActionInputsDefinition::relative)
      .field("internal", &griddly::ActionInputsDefinition::internal)
      .field("mapToGrid", &griddly::ActionInputsDefinition::mapToGrid);

  register_unordered_map<std::string, int32_t>("map<std::string, int32_t>");

  register_unordered_map<uint32_t, griddly::InputMapping>("map<uint32_t, griddly::InputMapping>");
  e::value_object<griddly::InputMapping>("InputMapping")
      .field("vectorToDest", &griddly::InputMapping::vectorToDest)
      .field("orientationVector", &griddly::InputMapping::orientationVector)
      .field("description", &griddly::InputMapping::description)
      .field("metaData", &griddly::InputMapping::metaData);


  e::enum_<griddly::ObserverType>("ObserverType")
      .value("VECTOR", griddly::ObserverType::VECTOR)
      .value("ASCII", griddly::ObserverType::ASCII)
      .value("ENTITY", griddly::ObserverType::ENTITY)
      .value("NONE", griddly::ObserverType::NONE);
}
