#pragma once
#include <memory>
#include <vector>
#include "GameProcess.hpp"

namespace griddy {
class TurnBasedGameProcess : public GameProcess {
 public:
  TurnBasedGameProcess(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<LevelGenerator> levelGenerator);
  ~TurnBasedGameProcess();

  virtual std::vector<int> performActions(int playerId, std::vector<std::shared_ptr<Action>> actions) override;

  virtual std::string getProcessName() const override;

 private:
  static const std::string name_;

};
}  // namespace griddy