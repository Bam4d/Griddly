#include "Griddle/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddle/Core/LevelGenerators/MapReader.hpp"
#include "gmock/gmock.h"

namespace griddle {

class MockMapReader : public MapReader {
 public:
  MockMapReader() : MapReader(nullptr) {}
  MockMapReader(std::shared_ptr<ObjectGenerator> objectGenerator) : MapReader(objectGenerator) {}

  MOCK_METHOD(void, parseFromStream, (std::istream & stream), ());
};
}  // namespace griddle