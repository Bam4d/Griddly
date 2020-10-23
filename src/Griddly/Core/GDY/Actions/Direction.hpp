#pragma once

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace griddly {
enum class Direction {
  UP,
  DOWN,
  LEFT,
  RIGHT,
  NONE
};

class DiscreteOrientation {
 public:
  DiscreteOrientation() {
    unitVector_ = {0, 0};
    direction_ = Direction::NONE;
  }
  
  DiscreteOrientation(glm::ivec2 vector) {
    unitVector_ = vector;

    if (unitVector_ == glm::ivec2(0, 0)) {
      direction_ = Direction::NONE;
    } else if (unitVector_.x == 1) {
      direction_ = Direction::RIGHT;
    } else if (unitVector_.x == -1) {
      direction_ = Direction::LEFT;
    } else if (unitVector_.y == 1) {
      direction_ = Direction::DOWN;
    } else if (unitVector_.y == -1) {
      direction_ = Direction::UP;
    } else {
      spdlog::error("Orientation is not discrete {0},{1}", vector.x, vector.y);
      throw std::runtime_error("Orientation of avatar is not discrete, unit vector can not be calculated");
    }
  }

  DiscreteOrientation(Direction direction) {
    direction_ = direction;

    switch (direction) {
      case Direction::NONE:
        unitVector_ = {0, 0};
        break;
      case Direction::UP:
        unitVector_ = {0, -1};
        break;
      case Direction::RIGHT:
        unitVector_ = {1, 0};
        break;
      case Direction::DOWN:
        unitVector_ = {0, 1};
        break;
      case Direction::LEFT:
        unitVector_ = {-1, 0};
        break;
    }
  }

  float getAngleRadians() {
    switch (direction_) {
      case Direction::NONE:
      case Direction::UP:
        return 0.0f;
      case Direction::RIGHT:
        return glm::pi<float>() / 2.0f;
      case Direction::DOWN:
        return glm::pi<float>();
      case Direction::LEFT:
        return 3 * glm::pi<float>() / 2.0;
      default:
        return 0.0f;
    }
  }

  glm::ivec2 getUnitVector() {
    return unitVector_;
  }

  // If the current direction is DOWN and the input vector is "right" we return "left" etc..
  glm::ivec2 getRelativeUnitVector(glm::ivec2 vector) {
    return vector * getRotationMatrix();
  }

  glm::imat2x2 getRotationMatrix() {
    switch (direction_) {
      case Direction::NONE:
      case Direction::UP:
        return {{1, 0}, {0, 1}};
      case Direction::RIGHT:
        return {{0, -1}, {1, 0}};
      case Direction::DOWN:
        return {{-1, 0}, {0, -1}};
      case Direction::LEFT:
        return {{0, 1}, {-1, 0}};
    }
  }

  Direction getDirection() {
    return direction_;
  }

 private:
  glm::ivec2 unitVector_ = {0, 0};
  Direction direction_ = Direction::NONE;
};
}  // namespace griddly