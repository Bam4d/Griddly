#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/LevelGenerators/MapReader.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockMapReader : public MapReader {
 public:
  MockMapReader() : MapReader(nullptr) {}
  MockMapReader(std::shared_ptr<ObjectGenerator> objectGenerator) : MapReader(objectGenerator) {}

  MOCK_METHOD(void, parseFromStream, (std::istream & stream), ());
};
}  // namespace griddly