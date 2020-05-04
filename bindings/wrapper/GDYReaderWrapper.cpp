#pragma once

#include "../../src/Griddy/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddy/Core/GDY/GDYFactory.hpp"

#include <sstream>

#include "../../src/Griddy/Core/Grid.hpp"
#include "GDYLevelWrapper.cpp"

namespace griddy {

class Py_GDYReaderWrapper {
 public:
  Py_GDYReaderWrapper() {
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYFile(std::string filename) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator));
    gdyFactory->initializeFromFile(filename);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory));
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYString(std::string string) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator));
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory));
  }


};
}  // namespace griddy