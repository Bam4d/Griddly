#pragma once

#include "../../src/Griddly/Core/GDY/TerminationGenerator.hpp"
#include "../../src/Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"

#include <sstream>

#include "../../src/Griddly/Core/Grid.hpp"
#include "GDYLevelWrapper.cpp"

namespace griddly {

class Py_GDYReaderWrapper {
 public:
  Py_GDYReaderWrapper(std::string resourceLocation) : resourceLocation_(resourceLocation) {
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYFile(std::string filename) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator));
    gdyFactory->initializeFromFile(filename);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory, resourceLocation_));
  }

  std::shared_ptr<Py_GDYLevelWrapper> loadGDYString(std::string string) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator));
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::shared_ptr<Py_GDYLevelWrapper>(new Py_GDYLevelWrapper(gdyFactory, resourceLocation_));
  }

  private:
  const std::string resourceLocation_;


};
}  // namespace griddly