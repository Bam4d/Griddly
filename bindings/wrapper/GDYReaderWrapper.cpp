#pragma once

#include <sstream>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddly/Core/GDY/TerminationGenerator.hpp"
#include "../../src/Griddly/Core/Grid.hpp"
#include "GridWrapper.cpp"

namespace griddly {

class Py_GDYReaderWrapper {
 public:
  Py_GDYReaderWrapper(std::string imagePath, std::string shaderPath)
      : imagePath_(imagePath), 
      shaderPath_(shaderPath) {
  }

  std::shared_ptr<Py_GridWrapper> loadGDYFile(std::string filename) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator));
    gdyFactory->initializeFromFile(filename);
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(gdyFactory, imagePath_, shaderPath_));
  }

  std::shared_ptr<Py_GridWrapper> loadGDYString(std::string string) {
    auto objectGenerator = std::shared_ptr<ObjectGenerator>(new ObjectGenerator());
    auto terminationGenerator = std::shared_ptr<TerminationGenerator>(new TerminationGenerator());
    auto gdyFactory = std::shared_ptr<GDYFactory>(new GDYFactory(objectGenerator, terminationGenerator));
    std::istringstream s(string);
    gdyFactory->parseFromStream(s);
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(gdyFactory, imagePath_, shaderPath_));
  }

 private:
  const std::string imagePath_;
  const std::string shaderPath_;
};
}  // namespace griddly