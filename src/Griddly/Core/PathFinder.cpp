#include "PathFinder.hpp"

namespace griddly {

PathFinder::PathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> impassableObjects) : 
    grid_(grid), impassableObjects_(impassableObjects)
{
}

}  // namespace griddly