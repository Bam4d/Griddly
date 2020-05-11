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

void MapReader::reset(std::shared_ptr<Grid> grid) {
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
  auto state = MapReaderState::READ_NORMAL;

  mapDescription_.empty();

  uint32_t rowCount = 0;
  uint32_t colCount = 0;
  uint32_t firstColCount = 0;

  std::string currentObjectName;

  char currentPlayerId[3];
  int playerIdIdx = 0;

  char prevChar;

  while (auto ch = stream.get()) {
    switch (ch) {
      case EOF:
        if (state == MapReaderState::READ_PLAYERID) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount);
          state = MapReaderState::READ_NORMAL;
        }
        width_ = firstColCount;

        if (prevChar != '\n') {
          rowCount += 1;
        }

        height_ = rowCount;
        spdlog::debug("Reached end of file.");
        return;

      case '\n':
        if (state == MapReaderState::READ_PLAYERID) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount);
          state = MapReaderState::READ_NORMAL;
          colCount++;
        }

        if (rowCount == 0) {
          firstColCount = colCount;
          spdlog::debug("Initial column count {0}", colCount);
        } else if (firstColCount != colCount) {
          throw std::invalid_argument(fmt::format("Invalid number of characters={0} in map row={1}, was expecting {2}", colCount, rowCount, firstColCount));
        }
        rowCount++;
        colCount = 0;
        prevChar = ch;
        break;

      // Do nothing on whitespace
      case ' ':
      case '\t':
        if (state == MapReaderState::READ_PLAYERID) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount);
          state = MapReaderState::READ_NORMAL;
          colCount++;
        }
        break;

      case '.':  // dots just signify an empty space
        if (state == MapReaderState::READ_PLAYERID) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount);
          state = MapReaderState::READ_NORMAL;
        }
        colCount++;
        prevChar = ch;
        break;

      default: {
        switch (state) {
          case MapReaderState::READ_NORMAL: {
            currentObjectName = objectGenerator_->getObjectNameFromMapChar(ch);
            state = MapReaderState::READ_PLAYERID;
            playerIdIdx = 0;
            memset(currentPlayerId, 0x00, 3);
          } break;
          case MapReaderState::READ_PLAYERID: {
            if (std::isdigit(ch)) {
              currentPlayerId[playerIdIdx] = ch;
              playerIdIdx++;
            } else {
              addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount);
              currentObjectName = objectGenerator_->getObjectNameFromMapChar(ch);
              playerIdIdx = 0;
              memset(currentPlayerId, 0x00, 3);
              colCount++;
            }
          } break;
        }
        prevChar = ch;
        break;
      }
    }
  }
}

void MapReader::addObject(std::string objectName, char* playerIdString, int playerIdStringLength, int x, int y) {
  auto playerId = playerIdStringLength > 0 ? atoi(playerIdString) : 0;
  GridInitInfo gridInitInfo;
  gridInitInfo.objectName = objectName;
  gridInitInfo.playerId = playerId;
  spdlog::debug("Adding object={0} with playerId={1} to location [{2}, {3}]", objectName, playerId, x, y);
  mapDescription_.insert({{x, y}, gridInitInfo});
}

}  // namespace griddy
