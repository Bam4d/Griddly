#pragma once
#include "../Object.hpp"

namespace griddy {
class Unit : public Object {
 public:
  Unit(int playerId, int initialHealth);
  virtual ~Unit() = 0;

  uint8_t getHealth() const;

  const uint8_t getMaxHealth() const;

  virtual bool onActionPerformed(std::shared_ptr<Object> sourceObject, std::shared_ptr<Action> action) override;

  virtual int onPerformAction(std::shared_ptr<Object> destinationObject, std::shared_ptr<Action> action) override;

 protected:
  const uint8_t playerId_;
  const uint8_t maxHealth_;
  uint8_t health_;

};
}  // namespace griddy