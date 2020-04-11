#pragma once
#include <memory>

#include "LevelGenerator.hpp"
#include "../Objects/ObjectTypes.hpp"

namespace griddy {

struct GridInitInfo {
    ObjectType objectType;
    int playerId;
}; 

class MapReader : public LevelGenerator {
 public:
  MapReader();
  ~MapReader() override;

  void parseFromStream(std::istream& stream);

  void initializeFromFile(std::string filename);

  void reset(std::shared_ptr<Grid>& grid) override;

 private:
  uint width_ = 0; 
  uint height_ = 0;
  std::unordered_map<GridLocation, GridInitInfo, GridLocation::Hash> mapDescription_;

  int parsePlayerId(std::istream& stream);
};
}  // namespace griddy