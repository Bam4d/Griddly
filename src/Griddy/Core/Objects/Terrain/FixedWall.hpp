#pragma once
#include "../Object.hpp"

namespace griddy {

class Action;

class FixedWall : public Object {
 public:
  ObjectType getObjectType() const override;

  std::string getDescription() const override;

  FixedWall();
  ~FixedWall() override;

 private:
  int value;

  static const ObjectType type;
};
}  // namespace griddy