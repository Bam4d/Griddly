#include "../Object.hpp"

namespace griddy {
class Unit : public Object {
 public:
  Unit(int playerId, int initialHealth);
  virtual ~Unit() = 0;

  virtual void performAction() = 0;

  int_least32_t getHealth() const;

  virtual bool onActionPerformed(std::shared_ptr<Object> sourceObject,
                         std::shared_ptr<Action> action) override;

  virtual bool onPerformAction(std::shared_ptr<Action> action) override;

 private:
  const int playerId_;
  int health_;
};
}  // namespace griddy