
#pragma once
#include "../Object.hpp"

namespace griddy {

class Action;

class Minerals : public Object {
 public:
  ObjectType getObjectType() const override;

  std::string getDescription() const override;

  bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  uint getValue() const;
  uint getMaxValue() const;

  Minerals(uint value); 
  ~Minerals() override;

 private:
  uint value_;
  const uint maxValue_;

  static const ObjectType type;
};
}  // namespace griddy