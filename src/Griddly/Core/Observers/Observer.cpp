#include "Observer.hpp"

namespace griddly {

Observer::Observer(std::shared_ptr<Grid> grid) : grid_(grid) {
}

void Observer::init(ObserverConfig observerConfig) {
  observerConfig_ = observerConfig;
}

void Observer::setAvatar(std::shared_ptr<Object> avatarObject) {
  avatarObject_= avatarObject;
}

void Observer::print(std::shared_ptr<uint8_t> observation) {
}

Observer::~Observer() {
}

}  // namespace griddly