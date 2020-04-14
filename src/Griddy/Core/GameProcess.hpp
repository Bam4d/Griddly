#pragma once

#include <memory>
#include <vector>
#include "Grid.hpp"
#include "Observers/Observer.hpp"

namespace griddy {

class Player;

class GameProcess : public std::enable_shared_from_this<GameProcess> {
 public:
  GameProcess(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<LevelGenerator> levelGenerator);

  virtual std::unique_ptr<uint8_t[]> observe(int playerId) const;

  virtual std::vector<int> performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) = 0;

  virtual void addPlayer(std::shared_ptr<Player> player);

  virtual void init();
  virtual void reset();

  virtual void startGame();
  virtual void endGame();

  virtual bool isStarted() const;

  virtual std::string getProcessName() const;

  std::shared_ptr<Grid> getGrid();
  std::shared_ptr<Observer> getObserver();
  std::shared_ptr<LevelGenerator> getLevelGenerator();

  virtual ~GameProcess() = 0;

 protected:
  std::vector<std::shared_ptr<Player>> players_;
  std::shared_ptr<Grid> grid_;
  std::shared_ptr<LevelGenerator> levelGenerator_;

  // Game process can have its own observer so we can monitor the game regardless of the player
  std::shared_ptr<Observer> observer_;

  

  bool isStarted_ = false;
  bool isInitialized_ = false;
};
}  // namespace griddy