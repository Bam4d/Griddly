#include "Griddly/Core/CollisionDetector.hpp"
#include "gmock/gmock.h"

namespace griddly {
class MockCollisionDetector : public CollisionDetector {
 public:
  MockCollisionDetector() : CollisionDetector(10, 10, 1) {}
  ~MockCollisionDetector() override = default;

  MOCK_METHOD(bool, upsert, (std::shared_ptr<Object> object), ());
  MOCK_METHOD(bool, remove, (std::shared_ptr<Object> object), ());
  MOCK_METHOD(SearchResult, search, (glm::ivec2 location), ());
};
}  // namespace griddly