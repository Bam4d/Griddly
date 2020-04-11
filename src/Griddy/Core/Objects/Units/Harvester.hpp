#pragma once
#include "Unit.hpp"

namespace griddy {
class Harvester : public Unit {
 public:
  ObjectType getObjectType() const override;

  std::string getDescription() const override;

  int onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) override;

  bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  bool canPerformAction(std::shared_ptr<Action> action) override;

  int getMinerals() const;

  Harvester(int playerId);
  ~Harvester() override;

 private:
  static const ObjectType type;
  int minerals_ = 0;

  const int maxMinerals_ = 2;
};

}  // namespace griddy