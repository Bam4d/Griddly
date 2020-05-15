#pragma once

#include "../../src/Griddle/Core/GDY/TerminationGenerator.hpp"
#include "../../src/Griddle/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddle/Core/GDY/GDYFactory.hpp"

#include <sstream>

#include "../../src/Griddle/Core/Grid.hpp"
#include "GDYLevelWrapper.cpp"

namespace griddle {

class Py_GDYReaderWrapper {
 public:
  Py_GDYReaderWrapper() {
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYFile(std::string filename) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator));
    gdyFactory->initializeFromFile(filename);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory));
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYString(std::string string) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator));
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory));
  }


};
}  // namespace griddle