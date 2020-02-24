#include "Harvester.hpp"

namespace griddy {

class Object;

const ObjectType Harvester::type = ObjectType::HARVESTER;


ObjectType Harvester::getType() const { return type; }

bool Harvester::onActionPerformed(std::shared_ptr<griddy::Object> sourceObject, std::shared_ptr<griddy::Action> action) {
    action->getActionTypeId() == std::hash<std::string>("")
}

int Harvester::increaseResources(int value) {
    resources += value;
}

Harvester::Harvester(int playerId) : Unit(playerId, 10) {}

Harvester::~Harvester() {}
}  // namespace griddy