#pragma once

#include <memory>
#include <string>

namespace griddy {

class Action;

enum ObjectType {
  RESOURCE=1,
  HARVESTER=2,

  MAIN_BASE=3
};

struct GridLocation {
  const int& x;
  const int& y;
};

class Object {
 public:

  GridLocation getLocation();

  void setLocation(GridLocation location);

  virtual ObjectType getType() const = 0;

  /**
   * An action has been performed on this object by another object, such as a
   * movement, harvest, attack etc
   */
  virtual bool onActionPerformed(std::shared_ptr<Object> sourceObject,
                                 std::shared_ptr<Action> action) = 0;

  virtual ~Object() = 0;

 private:
  int x = -1;
  int y = -1;
};

}  // namespace griddy