#include "TileObserver.hpp"
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

namespace griddy {

TileObserver::TileObserver() {}

TileObserver::~TileObserver() {}

std::unique_ptr<uint8_t[]> TileObserver::observe(int playerId, std::shared_ptr<Grid> grid) {
  int width = grid->getWidth();
  int height = grid->getHeight();

  std::unique_ptr<uint8_t[]> observation(new uint8_t[width * height]{});

  for(auto object : grid->getObjects()) {
    
    int x = object->getLocation().x;
    int y = object->getLocation().y;
    int idx = width*y + x;

    observation[idx] = object->getObjectType();

  }

  return std::move(observation);
}

void TileObserver::print(std::unique_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) {
  int width = grid->getWidth();
  int height = grid->getHeight();

  std::string printString;

  for (int h = height-1; h >= 0; h--) {
    printString += "[";
    for (int w = 0; w < width; w++) {
      int idx = h * width + w;
      printString += " " + std::to_string(observation[idx]) + " ";
    }
    printString += "]\n";
  }
  spdlog::debug("TileObservation: \n {0}", printString);
}

}  // namespace griddy