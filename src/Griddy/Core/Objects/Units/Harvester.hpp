#pragma once
#include "Unit.hpp"

namespace griddy {
class Harvester : public Unit {
 public:
  ObjectType getType() const override;

  int increaseResources(int value);

  bool onActionPerformed(std::shared_ptr<Object> sourceObject,
                         std::shared_ptr<Action> action) override;

  bool onPerformAction(std::shared_ptr<Action> action) override;

  Harvester(int playerId);
  ~Harvester() override;

 private:
  static const ObjectType type;
  int resources = 0;
};

}  // namespace griddy