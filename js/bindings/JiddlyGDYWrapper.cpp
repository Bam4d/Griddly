#include "JiddlyGDYWrapper.hpp"

JiddlyGDYWrapper::JiddlyGDYWrapper(std::shared_ptr<griddly::GDYFactory> gdyFactory)
    : gdyFactory_(gdyFactory) {
}

void JiddlyGDYWrapper::setMaxSteps(uint32_t maxSteps) {
  gdyFactory_->setMaxSteps(maxSteps);
}

uint32_t JiddlyGDYWrapper::getPlayerCount() const {
  return gdyFactory_->getPlayerCount();
}

std::string JiddlyGDYWrapper::getAvatarObject() const {
  return gdyFactory_->getAvatarObject();
}

e::val JiddlyGDYWrapper::getExternalActionNames() const {
  e::val js_actionNames = e::val::array();
  for (const auto& actionName : gdyFactory_->getExternalActionNames()) {
    js_actionNames.call<void>("push", actionName);
  }
  return js_actionNames;
}

uint32_t JiddlyGDYWrapper::getLevelCount() const {
  return gdyFactory_->getLevelCount();
}

griddly::ObserverType& JiddlyGDYWrapper::getObserverType(std::string observerName) {
  return gdyFactory_->getNamedObserverType(observerName);
}

e::val JiddlyGDYWrapper::getActionInputMappings() const {
  const auto& actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
  e::val js_actionInputsDefinitions = e::val::object();
  for (auto actionInputDefinitionPair : actionInputsDefinitions) {
    auto actionName = actionInputDefinitionPair.first;
    auto actionInputDefinition = actionInputDefinitionPair.second;

    auto internal = actionInputDefinition.internal;
    auto relative = actionInputDefinition.relative;
    auto mapToGrid = actionInputDefinition.mapToGrid;

    e::val js_actionInputsDefinition = e::val::object();

    js_actionInputsDefinition.set("Relative", relative);
    js_actionInputsDefinition.set("Internal", internal);
    js_actionInputsDefinition.set("MapToGrid", mapToGrid);

    e::val js_actionInputMappings = e::val::object();
    for (auto inputMapping : actionInputDefinition.inputMappings) {
      e::val js_actionInputMapping = e::val::object();
      auto inputId = inputMapping.first;
      auto actionInputMapping = inputMapping.second;

      js_actionInputMapping.set("Description", actionInputMapping.description);
      js_actionInputMapping.set("VectorToDest", actionInputMapping.vectorToDest);
      js_actionInputMapping.set("OrientationVector", actionInputMapping.orientationVector);
      js_actionInputMappings.set(std::to_string(inputId), js_actionInputMapping);
    }

    js_actionInputsDefinition.set("InputMappings", js_actionInputMappings);

    js_actionInputsDefinitions.set(actionName, js_actionInputsDefinition);
  }

  return js_actionInputsDefinitions;
}

std::shared_ptr<JiddlyGameWrapper> JiddlyGDYWrapper::createGame(std::string globalObserverName) {
  return std::make_shared<JiddlyGameWrapper>(JiddlyGameWrapper(globalObserverName, gdyFactory_));
}
