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

  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] float getAngleRadians() const {
    switch (direction_) {
      case Direction::NONE:
      case Direction::UP:
        return 0.0f;
      case Direction::RIGHT:
        return glm::pi<float>() / 2.0f;
      case Direction::DOWN:
        return glm::pi<float>();
      case Direction::LEFT:
        return 3.0f * glm::pi<float>() / 2.0f;
      default:
        return 0.0f;
    }
  }

  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] glm::ivec2 getUnitVector() const {
    return unitVector_;
  }

  std::string getName() {
    switch (direction_) {
      case Direction::NONE:
        return "NONE";
      case Direction::UP:
        return "UP";
      case Direction::RIGHT:
        return "RIGHT";
      case Direction::DOWN:
        return "DOWN";
      case Direction::LEFT:
        return "LEFT";
      default:
        return "NONE";
    }
  }

  // If the current direction is DOWN and the input vector is "right" we return "left" etc..
  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] glm::ivec2 getRelativeUnitVector(glm::ivec2 vector) const {
    return vector * getRotationMatrix();
  }

  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] glm::imat2x2 getRotationMatrix() const {
    switch (direction_) {
      default:
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

  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] Direction getDirection() const {
    return direction_;
  }

  inline bool operator==(const DiscreteOrientation& other) const {
    bool equal = direction_ == other.getDirection() &&
      unitVector_ == other.getUnitVector();

    return equal;
  }

 private:
  glm::ivec2 unitVector_ = {0, 0};
  Direction direction_ = Direction::NONE;
};
}  // namespace griddly