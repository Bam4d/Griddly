#pragma once

#include "../../src/Griddy/Core/LevelGenerators/MapReader.hpp"
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
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory());
    gdyFactory->initializeFromFile(filename);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory));
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYString(std::string string) {
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory());
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory));
  }


};
}  // namespace griddy