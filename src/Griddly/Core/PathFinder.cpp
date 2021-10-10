#include <PathFinder.hpp>

namespace griddly {

PathFinder::PathFinder(std::shared_ptr<Grid> grid, std::unordered_set<std::string> passableObjects, std::unordered_set<std::string> impassableObjects) : 
    grid_(grid), passableObjects_(passableObjects), impassableObjects_(impassableObjects)
{
}

}  // namespace griddly