#include "MapGenerator.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>
#include <utility>

namespace griddly {

MapGenerator::MapGenerator(uint32_t playerCount, std::shared_ptr<ObjectGenerator> objectGenerator) : playerCount_(playerCount), objectGenerator_(std::move(objectGenerator)) {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif
}

MapGenerator::~MapGenerator() = default;

void MapGenerator::reset(std::shared_ptr<Grid> grid) {
  grid->resetMap(width_, height_);

  for (auto objectDefinition : objectGenerator_->getObjectDefinitions()) {
    auto objectName = objectDefinition.second->objectName;
    if (objectName != "_empty" && objectName != "_boundary") {
      std::vector<std::string> objectVariableNames;
      for (auto variableNameIt : objectDefinition.second->variableDefinitions) {
        objectVariableNames.push_back(variableNameIt.first);
      }

      grid->initObject(objectName, objectVariableNames);
      spdlog::debug("Initializing object {0}", objectName);
    }
  }

  for (auto playerId = 0; playerId < playerCount_ + 1; playerId++) {
    auto defaultEmptyObject = objectGenerator_->newInstance("_empty", playerId, grid);
    auto defaultBoundaryObject = objectGenerator_->newInstance("_boundary", playerId, grid);
    grid->addPlayerDefaultBoundaryObject(defaultBoundaryObject);
    grid->addPlayerDefaultEmptyObject(defaultEmptyObject);
  }

  for (auto& actionTriggerDefinitionIt : objectGenerator_->getActionTriggerDefinitions()) {
    grid->addActionTrigger(actionTriggerDefinitionIt.first, actionTriggerDefinitionIt.second);
  }

  grid->setBehaviourProbabilities(objectGenerator_->getBehaviourProbabilities());

  for (auto& item : mapDescription_) {
    auto gridObjectData = item.second;
    auto location = item.first;

    for (auto& objectData : gridObjectData) {
      auto objectName = objectData.objectName;
      auto playerId = objectData.playerId;

      spdlog::debug("Adding object {0} to environment at location ({1},{2})", objectName, location[0], location[1]);
      auto object = objectGenerator_->newInstance(objectName, playerId, grid);
      spdlog::debug("Adding");
      grid->addObject(location, object, true, nullptr, DiscreteOrientation(objectData.initialDirection));
    }
  }
}

void MapGenerator::initializeFromFile(std::string filename) {
  spdlog::debug("Loading map file: {0}", filename);
  std::ifstream mapFile;
  mapFile.open(filename);
  parseFromStream(mapFile);
}

void MapGenerator::parseFromStream(std::istream& stream) {
  auto state = MapReaderState::READ_NORMAL;

  mapDescription_.clear();

  uint32_t rowCount = 0;
  uint32_t colCount = 0;
  uint32_t firstColCount = 0;

  std::string currentObjectName;

  char currentPlayerId[3];
  int playerIdIdx = 0;
  Direction currentDirection = Direction::NONE;

  char prevChar;

  while (auto ch = stream.get()) {
    switch (ch) {
      case EOF:
        if (state == MapReaderState::READ_PLAYERID || state == MapReaderState::READ_INITIAL_ORIENTATION) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount, currentDirection);
          currentDirection = Direction::NONE;
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
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount, currentDirection);
          state = MapReaderState::READ_NORMAL;
          currentDirection = Direction::NONE;
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
        if (state == MapReaderState::READ_PLAYERID || state == MapReaderState::READ_INITIAL_ORIENTATION) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount, currentDirection);
          state = MapReaderState::READ_NORMAL;
          currentDirection = Direction::NONE;
          colCount++;
        }
        break;

      case '.':  // dots just signify an empty space
        if (state == MapReaderState::READ_PLAYERID || state == MapReaderState::READ_INITIAL_ORIENTATION) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount, currentDirection);
          state = MapReaderState::READ_NORMAL;
          currentDirection = Direction::NONE;
          colCount++;
        }
        colCount++;
        prevChar = ch;
        break;

      case '/':
        if (state == MapReaderState::READ_PLAYERID || state == MapReaderState::READ_INITIAL_ORIENTATION) {
          addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount, currentDirection);
          state = MapReaderState::READ_NORMAL;
          currentDirection = Direction::NONE;
        }
        prevChar = ch;
        break;

      case '[':
        if (state == MapReaderState::READ_PLAYERID) {
          state = MapReaderState::READ_INITIAL_ORIENTATION;
        }
        prevChar = ch;
        break;

      case ']':
        if (state != MapReaderState::READ_INITIAL_ORIENTATION) {
          throw std::invalid_argument(fmt::format("Invalid closing bracket ']' for initial orientation in map row={0}", rowCount));
        }
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
              addObject(currentObjectName, currentPlayerId, playerIdIdx, colCount, rowCount, currentDirection);
              currentObjectName = objectGenerator_->getObjectNameFromMapChar(ch);
              playerIdIdx = 0;
              currentDirection = Direction::NONE;
              memset(currentPlayerId, 0x00, 3);
              colCount++;
            }
          } break;
          case MapReaderState::READ_INITIAL_ORIENTATION: {
            switch (ch) {
              case 'U':
                currentDirection = Direction::UP;
                break;
              case 'D':
                currentDirection = Direction::DOWN;
                break;
              case 'L':
                currentDirection = Direction::LEFT;
                break;
              case 'R':
                currentDirection = Direction::RIGHT;
                break;
              default:
                throw std::invalid_argument(fmt::format("Unknown direction character {0} at in map row={1}", ch, rowCount));
                break;
            }
          } break;
        }
        prevChar = ch;
        break;
      }
    }
  }
}

void MapGenerator::addObject(std::string& objectName, char* playerIdString, int playerIdStringLength, uint32_t x, uint32_t y, Direction direction) {
  auto playerId = playerIdStringLength > 0 ? atoi(playerIdString) : 0;
  GridInitInfo gridInitInfo;
  gridInitInfo.objectName = objectName;
  gridInitInfo.playerId = playerId;
  gridInitInfo.initialDirection = direction;
  spdlog::debug("Adding object={0} with playerId={1} to location [{2}, {3}]", objectName, playerId, x, y);

  auto location = glm::ivec2(x, y);
  mapDescription_[location].push_back(gridInitInfo);
}

}  // namespace griddly
