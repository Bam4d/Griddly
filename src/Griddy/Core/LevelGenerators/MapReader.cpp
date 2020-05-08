#include "MapReader.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>

namespace griddy {

MapReader::MapReader(std::shared_ptr<ObjectGenerator> objectGenerator) : objectGenerator_(objectGenerator) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
}

MapReader::~MapReader() {
}

void MapReader::reset(std::shared_ptr<Grid>& grid) {
  grid->init(width_, height_);

  for (auto& item : mapDescription_) {
    auto gridObjectData = item.second;
    auto location = item.first;

    auto objectName = gridObjectData.objectName;
    auto playerId = gridObjectData.playerId;

    auto object = objectGenerator_->newInstance(objectName);

    grid->initObject(playerId, location, object);
  }
}

void MapReader::initializeFromFile(std::string filename) {
  spdlog::debug("Loading map file: {0}", filename);
  std::ifstream mapFile;
  mapFile.open(filename);
  parseFromStream(mapFile);
}

void MapReader::parseFromStream(std::istream& stream) {
  mapDescription_.empty();

  uint32_t rowCount = 0;
  uint32_t colCount = 0;
  uint32_t prevColCount = 0;

  while (auto ch = stream.get()) {
    if (ch == EOF) {
      spdlog::debug("Reached end of file.");
      break;
    }

    switch (ch) {
      case '\n':
        if (rowCount == 0) {
          prevColCount = colCount;
          spdlog::debug("Initial column count {0}", colCount);
        } else if (prevColCount != colCount) {
          throw std::invalid_argument(fmt::format("Invalid number of characters={0} in map row={1}, was expecting {2}", colCount, rowCount, prevColCount));
        }
        rowCount++;
        colCount = 0;
        break;

      // Do nothing on whitespace
      case ' ':
      case '\t':
        break;

      case '.':  // dots just signify an empty space
        colCount++;
        break;

      default: {
        auto objectName = objectGenerator_->getObjectNameFromMapChar(ch);
        int playerId = parsePlayerId(stream);
        spdlog::debug("Player {0} {1} at [{2}, {3}] ", playerId, objectName, colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {objectName, playerId}});
        colCount++;

      } break;
    }
  }

  width_ = prevColCount;
  height_ = rowCount;

  spdlog::debug("Level loaded! dimensions: [{0}, {1}] ", width_, height_);
}

int MapReader::parsePlayerId(std::istream& stream) {
  char idStr[3];

  stream.get(idStr, 2);

  if (idStr[0] == ' ') {
    return 0;
  }

  auto playerId = atoi(idStr);
  // if (playerId == 0) {
  //   throw std::runtime_error("Player Ids in map files must be larger than 1. 0 is reserved for neutral game objects.");
  // }
  return playerId;
}

}  // namespace griddy