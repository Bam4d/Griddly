#pragma once
#include "../Object.hpp"

namespace griddy {

class Action;

class FixedWall : public Object {
 public:
  ObjectType getObjectType() const override;

  std::string getDescription() const override;

  bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  FixedWall();
  ~FixedWall() override;

 private:
  int value;

  static const ObjectType type;
};
}  // namespace griddy