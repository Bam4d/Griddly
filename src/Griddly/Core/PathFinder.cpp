#include "PathFinder.hpp"
#include "Grid.hpp"

namespace griddly {

PathFinder::PathFinder(std::shared_ptr<Grid> grid, std::set<std::string> impassableObjects) : 
    grid_(grid), impassableObjects_(impassableObjects)
{
}

}  // namespace griddly