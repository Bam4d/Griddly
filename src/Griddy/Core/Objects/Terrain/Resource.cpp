#include "Resource.hpp"

namespace griddy {

const ObjectType Resource::type = ObjectType::RESOURCE;

ObjectType Resource::getType() const { return type; }

bool Resource::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject,
                                 std::shared_ptr<griddy::Action> action) {}

Resource::Resource(int value) {
  this->value = value;
}

Resource::~Resource() {}
}  // namespace griddy