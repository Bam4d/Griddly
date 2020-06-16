#pragma once
#include <memory>

#include "LevelGenerator.hpp"
#include "../GDY/Objects/ObjectGenerator.hpp"

namespace griddly {

struct GridInitInfo {
    std::string objectName;
    int playerId;
};

enum class MapReaderState {
  READ_NORMAL,
  READ_PLAYERID
};

class MapReader : public LevelGenerator {
 public:
  MapReader(std::shared_ptr<ObjectGenerator> objectGenerator);
  ~MapReader() override;

  virtual void parseFromStream(std::istream& stream);

  void initializeFromFile(std::string filename);

  virtual std::unordered_map<uint32_t, std::shared_ptr<Object>> reset(std::shared_ptr<Grid> grid) override;

 private:
  uint32_t width_ = 0; 
  uint32_t height_ = 0;
  std::unordered_map<GridLocation, GridInitInfo, GridLocation::Hash> mapDescription_;

  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  void addObject(std::string objectName, char* playerIdString, int playerIdStringLength, uint32_t x, uint32_t y);
};
}  // namespace griddly