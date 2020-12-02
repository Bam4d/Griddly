#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/LevelGenerators/MapReader.hpp"
#include "gmock/gmock.h"

namespace griddly {

class MockLevelGenerator : public LevelGenerator {
 public:
  MockLevelGenerator() : LevelGenerator() {}

  MOCK_METHOD(void, parseFromStream, (std::istream & stream), ());

  MOCK_METHOD(void, reset, (std::shared_ptr<Grid> grid), (override));
};
}  // namespace griddly