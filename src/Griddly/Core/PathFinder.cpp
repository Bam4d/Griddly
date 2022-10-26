#include "PathFinder.hpp"

#include <utility>

#include <utility>

#include "Grid.hpp"

namespace griddly {

PathFinder::PathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects, PathFinderMode mode) : grid_(std::move(grid)), impassableObjects_(std::move(impassableObjects)), mode_(mode) {
}

}  // namespace griddly