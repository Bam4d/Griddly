#pragma once
#include <memory>

#include "../GDY/Objects/ObjectGenerator.hpp"
#include "LevelGenerator.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace griddly {

struct GridInitInfo {
  std::string objectName;
  int32_t playerId;
  int32_t zIdx;
  Direction initialDirection;
};

enum class MapReaderState {
  READ_NORMAL,
  READ_PLAYERID,
  READ_INITIAL_ORIENTATION,
};

class MapGenerator : public LevelGenerator {
 public:
  MapGenerator(uint32_t playerCount, std::shared_ptr<ObjectGenerator> objectGenerator);
  ~MapGenerator() override;

  virtual void parseFromStream(std::istream& stream);

  void initializeFromFile(std::string filename);

  void reset(std::shared_ptr<Grid> grid) override;

 private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  const uint32_t playerCount_;
  std::unordered_map<glm::ivec2, std::vector<GridInitInfo>> mapDescription_;

  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  void addObject(std::string& objectName, char* playerIdString, int playerIdStringLength, uint32_t x, uint32_t y, Direction direction);
};
}  // namespace griddly