#pragma once

#include <memory>
#include <sstream>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/Grid.hpp"
#include "GDYWrapper.cpp"

namespace griddly {

class Py_GriddlyLoaderWrapper {
 public:
  Py_GriddlyLoaderWrapper(std::string imagePath, std::string shaderPath)
      : resourceConfig_({imagePath, shaderPath}) {
  }

  std::shared_ptr<Py_GDYWrapper> loadGDYFile(std::string filename) {
    auto objectGenerator = std::make_shared<ObjectGenerator>();
    auto terminationGenerator = std::make_shared<TerminationGenerator>();
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator, resourceConfig_));
    gdyFactory->initializeFromFile(filename);
    return std::shared_ptr<Py_GDYWrapper>(new Py_GDYWrapper(gdyFactory));
  }

  std::shared_ptr<Py_GDYWrapper> loadGDYString(std::string string) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator, resourceConfig_));
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::shared_ptr<Py_GDYWrapper>(new Py_GDYWrapper(gdyFactory));
  }

 private:
  const ResourceConfig resourceConfig_;
};
}  // namespace griddly