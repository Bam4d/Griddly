#include "Resource.hpp"
#include "../Units/Harvester.hpp"

namespace griddy {

const ObjectType Resource::type = ObjectType::RESOURCE;

ObjectType Resource::getType() const { return type; }

bool Resource::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
  // set the resources on the source object
  if (sourceObject->getType() == ObjectType::HARVESTER) {
    auto harvester = std::dynamic_pointer_cast<Harvester> (sourceObject);

    // If this resource has no value, then the action cannot be performed
    if(value == 0) {
      return false;
    }
    
    value -= 1;
    harvester->increaseResources(1);
    return true;
  }

  return false;
}

Resource::Resource(int value) {
  this->value = value;
}

Resource::~Resource() {}
}  // namespace griddy