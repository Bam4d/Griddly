#pragma once

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace griddly {
enum class Direction {
  NONE,
  UP,
  RIGHT,
  DOWN,
  LEFT
};

class DiscreteOrientation {
 public:
  DiscreteOrientation() {
    direction_ = Direction::NONE;
  }

  DiscreteOrientation(glm::ivec2 vector) {
    *dx_ = vector.x;
    *dy_ = vector.y;
    setOrientation(vector);
  }

  DiscreteOrientation(Direction direction) {
    setOrientation(direction);
  }

  void setOrientation(Direction direction) {
    direction_ = direction;

    switch (direction) {
      case Direction::NONE:
        *dx_ = 0;
        *dy_ = 0;
        break;
      case Direction::UP:
        *dx_ = 0;
        *dy_ = -1;
        break;
      case Direction::RIGHT:
        *dx_ = 1;
        *dy_ = 0;
        break;
      case Direction::DOWN:
        *dx_ = 0;
        *dy_ = 1;
        break;
      case Direction::LEFT:
        *dx_ = -1;
        *dy_ = 0;
        break;
    }
  }

  void setOrientation(glm::ivec2 vector) {
    *dx_ = vector.x;
    *dy_ = vector.y;
    if (vector == glm::ivec2(0, 0)) {
      direction_ = Direction::NONE;
    } else if (vector.x == 1) {
      direction_ = Direction::RIGHT;
    } else if (vector.x == -1) {
      direction_ = Direction::LEFT;
    } else if (vector.y == 1) {
      direction_ = Direction::DOWN;
    } else if (vector.y == -1) {
      direction_ = Direction::UP;
    } else {
      spdlog::error("Orientation is not discrete {0},{1}", vector.x, vector.y);
      throw std::runtime_error("Orientation of avatar is not discrete, unit vector can not be calculated");
    }
  }

  float getAngleRadians() const {
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

  const std::shared_ptr<int32_t>& getDy() const {
    return dy_;
  }

  const std::shared_ptr<int32_t>& getDx() const {
    return dx_;
  }

  const glm::ivec2 getUnitVector() const {
    return glm::ivec2(*dx_, *dy_);
  }

  const std::string getName() const {
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
  glm::ivec2 getRelativeUnitVector(glm::ivec2 vector) const {
    return vector * getRotationMatrix();
  }

  glm::imat2x2 getRotationMatrix() const {
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

  Direction getDirection() const {
    return direction_;
  }

  inline bool operator==(const DiscreteOrientation& other) const {
    bool equal = direction_ == other.getDirection() &&
                 *dx_ == *other.getDx() &&
                 *dy_ == *other.getDy();

    return equal;
  }

 private:
  std::shared_ptr<int32_t> dx_ = std::make_shared<int32_t>(0);
  std::shared_ptr<int32_t> dy_ = std::make_shared<int32_t>(0);
  Direction direction_ = Direction::NONE;
};
}  // namespace griddly