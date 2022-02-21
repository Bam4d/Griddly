#include "PathFinder.hpp"

#include <utility>

#include <utility>

#include "Grid.hpp"

namespace griddly {

PathFinder::PathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects) : grid_(std::move(std::move(grid))), impassableObjects_(std::move(std::move(impassableObjects))) {
}

}  // namespace griddly