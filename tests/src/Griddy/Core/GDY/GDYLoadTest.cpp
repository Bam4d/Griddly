#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <memory>
#include <functional>
#include <iostream>

#include "Griddy/Core/GDY/GDYFactory.hpp"
#include "Griddy/Core/GDY/Actions/Action.hpp"

namespace griddy {

// TEST(GDYFactoryTest, loadGDYFile) {
//     auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory());
//     gdyFactory->initializeFromFile("resources/games/basicRTS.yaml");

//     auto grid = std::shared_ptr<Grid>(new Grid());
//     grid->init(10,10);

//     auto objectGenerator = gdyFactory->getObjectGenerator();

//     auto object = objectGenerator->newInstance("puncher");

//     grid->initObject(1, {5, 5}, object);

//     auto action = std::shared_ptr<Action>(new Action("move", {5, 5}, Direction::UP));

//     grid->performActions(1, {action});
// };

TEST(GDYFactoryTest, loadGDYFile) {
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory());
    gdyFactory->initializeFromFile("resources/games/basicRTS.yaml");

    auto grid = std::shared_ptr<Grid>(new Grid());
    std::shared_ptr<BlockObserver> blockObserver = std::shared_ptr<BlockObserver>(new BlockObserver(grid, gdyFactory->getTileSize(), gdyFactory->getBlockObserverDefinitions()));
    gdyFactory->loadLevel(0);
    gdyFactory->getLevelGenerator()->reset(grid);

    blockObserver->init(grid->getWidth(), grid->getHeight());
    blockObserver->reset();

    auto action = std::shared_ptr<Action>(new Action("move", {5, 5}, Direction::UP));

    grid->performActions(1, {action});

    
};


// class Scope {
//     public:
//      std::function<uint(std::shared_ptr<uint> argVal)> getFunction(std::shared_ptr<uint> scopeVal) {
//        auto functionLocal = std::make_shared<uint>(2);
//        return [this, scopeVal, functionLocal](std::shared_ptr<uint> arg) {
//          return (*arg) * (*scopeVal) * localVal * (*functionLocal);
//        };
//      }

//      std::function<uint(std::shared_ptr<uint> argVal)> getFunction2(std::shared_ptr<uint> scopeVal) {
//        auto functionLocal = std::make_shared<uint>(5);
//        return [this, scopeVal, functionLocal](std::shared_ptr<uint> arg) {
//          return (*arg) * (*scopeVal) * localVal * (*functionLocal);
//        };
//      }
//     private:
//         const uint localVal = 5;
// };

// TEST(GDYFactoryTest, testFunction) {

//     auto scope = std::shared_ptr<Scope>(new Scope());

//     std::string key = "key";

//     std::unordered_map<std::string, std::vector<std::function<uint(std::shared_ptr<uint> argVal)>>> functionsList;

//     auto fun1 = scope->getFunction(std::make_shared<uint>(1));
//     auto fun2 = scope->getFunction2(std::make_shared<uint>(2));
//     auto fun3 = scope->getFunction(std::make_shared<uint>(3));

//     functionsList[key].push_back(std::move(fun1));
//     functionsList[key].push_back(fun2);
//     functionsList[key].push_back(fun3);

//     for (auto fun : functionsList[key]) {
//         auto res = fun(std::make_shared<uint>(10));
//         std::cout << res << std::endl;
//     }
// };

}