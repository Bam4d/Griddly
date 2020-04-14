#pragma once
#include "../Object.hpp"

namespace griddy {
class Unit : public Object {
 public:
  Unit(int initialHealth);
  virtual ~Unit() = 0;

  uint getHealth() const;

  const uint getMaxHealth() const;

  virtual bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  virtual int onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) override;

 protected:
  const uint maxHealth_;
  uint health_;

};
}  // namespace griddy