#pragma once

#include <sstream>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddly/Core/GDY/TerminationGenerator.hpp"
#include "../../src/Griddly/Core/Grid.hpp"
#include "GDYWrapper.cpp"

namespace griddly {


class Py_GriddlyLoaderWrapper {
 public:
  Py_GriddlyLoaderWrapper(std::string gdyPath, std::string imagePath, std::string shaderPath)
      : resourceConfig_({gdyPath, imagePath, shaderPath}) {
  }

  std::shared_ptr<Py_GDYWrapper> loadGDYFile(std::string filename) {
    auto objectGenerator = std::make_shared<ObjectGenerator>(ObjectGenerator());
    auto terminationGenerator = std::make_shared<TerminationGenerator>(TerminationGenerator());
    auto gdyFactory = std::make_shared<GDYFactory>(GDYFactory(objectGenerator, terminationGenerator, resourceConfig_));
    gdyFactory->initializeFromFile(filename);
    return std::make_shared<Py_GDYWrapper>(Py_GDYWrapper(gdyFactory));
  }

  std::shared_ptr<Py_GDYWrapper> loadGDYString(std::string string) {
    auto objectGenerator = std::make_shared<ObjectGenerator>(ObjectGenerator());
    auto terminationGenerator = std::make_shared<TerminationGenerator>(TerminationGenerator());
    auto gdyFactory = std::make_shared<GDYFactory>(GDYFactory(objectGenerator, terminationGenerator, resourceConfig_));
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::make_shared<Py_GDYWrapper>(Py_GDYWrapper(gdyFactory));
  }

 private:
  const ResourceConfig resourceConfig_;
};
}  // namespace griddly