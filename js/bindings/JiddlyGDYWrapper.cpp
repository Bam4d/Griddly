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

std::vector<std::string> JiddlyGDYWrapper::getExternalActionNames() const {
  return gdyFactory_->getExternalActionNames();
}

uint32_t JiddlyGDYWrapper::getLevelCount() const {
  return gdyFactory_->getLevelCount();
}

griddly::ObserverType& JiddlyGDYWrapper::getObserverType(std::string observerName) {
  return gdyFactory_->getNamedObserverType(observerName);
}

std::unordered_map<std::string, griddly::ActionInputsDefinition> JiddlyGDYWrapper::getActionInputMappings() const {
  return gdyFactory_->getActionInputsDefinitions();
}

std::shared_ptr<JiddlyGameWrapper> JiddlyGDYWrapper::createGame(std::string globalObserverName) {
  return std::make_shared<JiddlyGameWrapper>(JiddlyGameWrapper(globalObserverName, gdyFactory_));
}
