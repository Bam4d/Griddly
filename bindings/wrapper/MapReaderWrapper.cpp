#pragma once

#include "../../src/Griddy/Core/LevelGenerators/MapReader.hpp"

#include <sstream>

#include "../../src/Griddy/Core/Grid.hpp"
#include "GridWrapper.cpp"

namespace griddy {

class Py_MapReaderWrapper {
 public:
  Py_MapReaderWrapper() {
  }

  std::shared_ptr<Py_GridWrapper> loadMapFile(std::string filename) {
    auto mapReader = std::shared_ptr<MapReader>(new MapReader());
    mapReader->initializeFromFile(filename);
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(mapReader));
  }

  std::shared_ptr<Py_GridWrapper> loadMapString(std::string string) {
    auto mapReader = std::shared_ptr<MapReader>(new MapReader());
    std::istringstream s(string);
    mapReader->parseFromStream(s);
    return std::shared_ptr<Py_GridWrapper>(new Py_GridWrapper(mapReader));
  }
};
}  // namespace griddy
