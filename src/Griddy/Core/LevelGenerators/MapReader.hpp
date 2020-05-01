#pragma once
#include <memory>

#include "LevelGenerator.hpp"
#include "../GDY/Objects/ObjectGenerator.hpp"

namespace griddy {

struct GridInitInfo {
    std::string objectName;
    int playerId;
}; 

class MapReader : public LevelGenerator {
 public:
  MapReader(std::shared_ptr<ObjectGenerator> objectGenerator);
  ~MapReader() override;

  void parseFromStream(std::istream& stream);

  void initializeFromFile(std::string filename);

  void reset(std::shared_ptr<Grid>& grid) override;

 private:
  uint32_t width_ = 0; 
  uint32_t height_ = 0;
  std::unordered_map<GridLocation, GridInitInfo, GridLocation::Hash> mapDescription_;

  const std::shared_ptr<ObjectGenerator> objectGenerator_;

  int parsePlayerId(std::istream& stream);
};
}  // namespace griddy