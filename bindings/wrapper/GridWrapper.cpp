#include <memory>

#include "../../src/Griddy/Core/Grid.hpp"
#include "../../src/Griddy/Core/Observers/BlockObserver.hpp"
#include "../../src/Griddy/Core/Observers/TileObserver.hpp"
#include "../../src/Griddy/Core/Players/StepPlayer.hpp"
#include "../../src/Griddy/Core/TurnBasedGameProcess.hpp"
#include "GameProcessWrapper.cpp"
#include "StepPlayerWrapper.cpp"

namespace griddy {

enum ObserverType { BLOCK_2D,
                    TILE };

class Py_GridWrapper {
 public:
  Py_GridWrapper(int width, int height) : grid_(std::move(std::shared_ptr<Grid>(new Grid(width, height)))) {
  }

  int getWidth() const {
    return grid_->getWidth();
  };

  int getHeight() const {
    return grid_->getHeight();
  }

  void addObject(std::shared_ptr<Py_PlayerWrapper> player, int startX, int startY) {
  }

  std::shared_ptr<Py_PlayerWrapper> addPlayer(std::string playerName) {
    if (isBuilt_) {
      throw std::invalid_argument("Cannot add players after the environment has been built with buildEnv");
    }
    // auto newPlayer = std::shared_ptr<Py_PlayerWrapper>(new Py_PlayerWrapper(++playerCount, startX, startY, ))
    auto nextPlayerId = players_.size();
    auto player = std::shared_ptr<Py_PlayerWrapper>(new Py_PlayerWrapper(nextPlayerId, playerName));
    players_.push_back(player);
    return player;
  }

  std::shared_ptr<Py_GameProcessWrapper> startGame(ObserverType observerType) {
    if (isBuilt_) {
      throw std::invalid_argument("Cannot build environment more than once.");
    }

    switch (observerType) {
      case BLOCK_2D:
        observer_ = std::shared_ptr<BlockObserver>(new BlockObserver(24));
        break;
      case TILE:
        observer_ = std::shared_ptr<TileObserver>(new TileObserver());
        break;
    }

    std::vector<std::shared_ptr<Player>> internalPlayers;
    for (auto &p : players_) {
      internalPlayers.push_back(p->unwrapped());
    }

    auto gameProcess = std::shared_ptr<griddy::TurnBasedGameProcess>(new griddy::TurnBasedGameProcess(internalPlayers, observer_, grid_));

    isBuilt_ = true;
    gameProcess->startGame();

    return std::shared_ptr<Py_GameProcessWrapper>(new Py_GameProcessWrapper(gameProcess));
  }

 private:
  const std::shared_ptr<Grid> grid_;
  std::shared_ptr<Observer> observer_;
  std::vector<std::shared_ptr<Py_PlayerWrapper>> players_;

  bool isBuilt_;
};

}  // namespace griddy