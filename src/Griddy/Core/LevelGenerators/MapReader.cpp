#include "MapReader.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>

#include "../Objects/Object.hpp"
#include "../Objects/Terrain/FixedWall.hpp"
#include "../Objects/Terrain/Minerals.hpp"
#include "../Objects/Terrain/PushableWall.hpp"
#include "../Objects/Units/Harvester.hpp"
#include "../Objects/Units/Puncher.hpp"
#include "../Objects/Units/Pusher.hpp"
#include "../Objects/Units/Base.hpp"

namespace griddy {

MapReader::MapReader() {
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

    auto objectType = gridObjectData.objectType;
    auto playerId = gridObjectData.playerId;

    std::shared_ptr<Object> object;

    switch (objectType) {
        case BASE:
        object = std::shared_ptr<Base>(new Base(playerId));
        break;
      case HARVESTER:
        object = std::shared_ptr<Harvester>(new Harvester(playerId));
        break;
      case PUNCHER:
        object = std::shared_ptr<Puncher>(new Puncher(playerId));
        break;
      case PUSHER:
        object = std::shared_ptr<Pusher>(new Pusher(playerId));
        break;
      case MINERALS:
        object = std::shared_ptr<Minerals>(new Minerals(10));
        break;
      case PUSHABLE_WALL:
        object = std::shared_ptr<PushableWall>(new PushableWall());
        break;
      case FIXED_WALL:
        object = std::shared_ptr<FixedWall>(new FixedWall());
        break;
    }

    grid->initObject(location, object);
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

  uint rowCount = 0;
  uint colCount = 0;
  uint prevColCount = 0;

  while (auto ch = stream.get()) {

    if(ch == EOF) {
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
      case 'B': {
        int playerId = parsePlayerId(stream);
        spdlog::debug("Player {0} BASE at [{1}, {2}] ", playerId, colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::BASE, playerId}});
        colCount++;
        break;
      }
      case 'H': {
        int playerId = parsePlayerId(stream);
        spdlog::debug("Player {0} HARVESTER at [{1}, {2}] ", playerId, colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::HARVESTER, playerId}});
        colCount++;
        break;
      }
      case 'p': {
        int playerId = parsePlayerId(stream);
        spdlog::debug("Player {0} PUNCHER at [{1}, {2}] ", playerId, colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::PUNCHER, playerId}});
        colCount++;
        break;
      }
      case 'P': {
        int playerId = parsePlayerId(stream);
        spdlog::debug("Player {0} PUSHER at [{1}, {2}] ", playerId, colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::PUSHER, playerId}});
        colCount++;
        break;
      }
      case 'M':
        spdlog::debug("MINERALS at [{0}, {1}] ", colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::MINERALS, -1}});
        colCount++;
        break;
      case 'w':
        spdlog::debug("PUSHABLE_WALL at [{0}, {1}] ", colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::PUSHABLE_WALL, -1}});
        colCount++;
        break;
      case 'W':
        spdlog::debug("FIXED_WALL at [{0}, {1}] ", colCount, rowCount);
        mapDescription_.insert({{colCount, rowCount}, {ObjectType::FIXED_WALL, -1}});
        colCount++;
        break;
      case '.':  // dots just signify an empty space
        colCount++;
        break;
      default:  // In the default case, where the character is not recognised we just ignore it (its probably whitespace)
        break;
    }
  }

  width_ = prevColCount;
  height_ = rowCount;

  spdlog::debug("Level loaded! dimensions: [{0}, {1}] ", width_, height_);
}

int MapReader::parsePlayerId(std::istream& stream) {
  char idStr[3];
  stream.get(idStr, 2);
  return atoi(idStr);
}

}  // namespace griddy