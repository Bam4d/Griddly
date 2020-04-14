#pragma once
#include "Unit.hpp"

namespace griddy {
class Base : public Unit {
 public:
  ObjectType getObjectType() const override;

  std::string getDescription() const override;

  bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  int getMinerals() const;

  Base();
  ~Base() override;

 private:
  static const ObjectType type;
  int minerals_ = 0;

};

}  // namespace griddy