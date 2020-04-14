#pragma once
#include "Unit.hpp"

namespace griddy {
class Puncher : public Unit {
 public:
  ObjectType getObjectType() const override;

  std::string getDescription() const override;

  int onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) override;

  bool canPerformAction(std::shared_ptr<Action> action) override;

  bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  Puncher();
  ~Puncher() override;

 private:
  static const ObjectType type;
  int resources = 0;
};

}  // namespace griddy