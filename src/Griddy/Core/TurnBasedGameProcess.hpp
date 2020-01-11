#pragma once
#include <memory>
#include <vector>
#include "GameProcess.hpp"

namespace griddy {
class TurnBasedGameProcess : public GameProcess {
 public:
  TurnBasedGameProcess(std::vector<std::shared_ptr<Player>> players, std::shared_ptr<Observer> observer, std::shared_ptr<Grid> grid);
  ~TurnBasedGameProcess();

  virtual void startGame() const;
  virtual void endGame() const;

  virtual std::string getProcessName() const;

 private:
  static const std::string name;
};
}  // namespace griddy