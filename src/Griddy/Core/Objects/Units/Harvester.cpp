#include "Harvester.hpp"

namespace griddy {

class Object;

const ObjectType Harvester::type = ObjectType::HARVESTER;

ObjectType Harvester::getType() const { return type; }

bool Harvester::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject,
                                 std::shared_ptr<griddy::Action> action) {}

Harvester::Harvester() {}

Harvester::~Harvester() {}
}  // namespace griddy