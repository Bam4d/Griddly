#pragma once
#include "../Object.hpp"

namespace griddy {
class Harvester : public Object {
 public:
  ObjectType getType() const override;

  bool onActionPerformed(std::shared_ptr<Object> sourceObject,
                         std::shared_ptr<Action> action) override;

  Harvester();
  ~Harvester() override;

 private:
  static const ObjectType type;
};

}  // namespace griddy