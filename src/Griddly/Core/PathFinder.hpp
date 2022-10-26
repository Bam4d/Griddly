#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <set>
#include <string>

namespace griddly {

class Grid;

struct SearchOutput {
  uint32_t actionId = 0;
};

enum class PathFinderMode {
  SEEK,
  FLEE
};

class PathFinder {
 public:
  PathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects, PathFinderMode mode);
  virtual ~PathFinder() = default;

  virtual SearchOutput search(glm::ivec2 startLocation, glm::ivec2 endLocation, glm::ivec2 startOrientationVector, uint32_t maxDepth) = 0;

 protected:
  const std::shared_ptr<Grid> grid_;
  std::set<std::string> impassableObjects_;
  const PathFinderMode mode_;
};

}  // namespace griddly