#include "GriddlyJSGDYWrapper.hpp"

GriddlyJSGDYWrapper::GriddlyJSGDYWrapper(std::shared_ptr<griddly::GDYFactory> gdyFactory)
    : gdyFactory_(gdyFactory) {
}

void GriddlyJSGDYWrapper::setMaxSteps(uint32_t maxSteps) {
  gdyFactory_->setMaxSteps(maxSteps);
}

uint32_t GriddlyJSGDYWrapper::getPlayerCount() const {
  return gdyFactory_->getPlayerCount();
}

std::string GriddlyJSGDYWrapper::getAvatarObject() const {
  return gdyFactory_->getAvatarObject();
}

e::val GriddlyJSGDYWrapper::getExternalActionNames() const {
  e::val js_actionNames = e::val::array();
  for (const auto& actionName : gdyFactory_->getExternalActionNames()) {
    js_actionNames.call<void>("push", actionName);
  }
  return js_actionNames;
}

uint32_t GriddlyJSGDYWrapper::getLevelCount() const {
  return gdyFactory_->getLevelCount();
}

griddly::ObserverType& GriddlyJSGDYWrapper::getObserverType(std::string observerName) {
  return gdyFactory_->getNamedObserverType(observerName);
}

e::val GriddlyJSGDYWrapper::getActionInputMappings() const {
  const auto& actionInputsDefinitions = gdyFactory_->getActionInputsDefinitions();
  e::val js_actionInputsDefinitions = e::val::object();
  for (auto actionInputDefinitionPair : actionInputsDefinitions) {
    auto actionName = actionInputDefinitionPair.first;
    auto actionInputDefinition = actionInputDefinitionPair.second;

    auto internal = actionInputDefinition.internal;
    auto relative = actionInputDefinition.relative;
    auto mapToGrid = actionInputDefinition.mapToGrid;

    e::val js_actionInputsDefinition = e::val::object();

    js_actionInputsDefinition.set("relative", relative);
    js_actionInputsDefinition.set("internal", internal);
    js_actionInputsDefinition.set("mapToGrid", mapToGrid);

    e::val js_actionInputMappings = e::val::object();
    for (auto inputMapping : actionInputDefinition.inputMappings) {
      e::val js_actionInputMapping = e::val::object();
      auto inputId = inputMapping.first;
      auto actionInputMapping = inputMapping.second;

      js_actionInputMapping.set("description", actionInputMapping.description);
      js_actionInputMapping.set("vectorToDest", actionInputMapping.vectorToDest);
      js_actionInputMapping.set("orientationVector", actionInputMapping.orientationVector);
      js_actionInputMappings.set(std::to_string(inputId), js_actionInputMapping);
    }

    js_actionInputsDefinition.set("inputMappings", js_actionInputMappings);

    js_actionInputsDefinitions.set(actionName, js_actionInputsDefinition);
  }

  return js_actionInputsDefinitions;
}

std::shared_ptr<GriddlyJSGameWrapper> GriddlyJSGDYWrapper::createGame(std::string globalObserverName) {
  return std::make_shared<GriddlyJSGameWrapper>(GriddlyJSGameWrapper(globalObserverName, gdyFactory_));
}
