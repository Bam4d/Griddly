#pragma once
#include <memory>
#include <vector>

#include "GameProcess.hpp"
#include "Util/util.hpp"

namespace griddly {

class GDYFactory;

class TurnBasedGameProcess : public GameProcess {
 public:
  TurnBasedGameProcess(std::string globalObserverName, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<Grid> grid);
  ~TurnBasedGameProcess() override;

  ActionResult performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions, bool updateTicks = true) override;

  std::string getProcessName() const override;

  void setTerminationHandler(std::shared_ptr<TerminationHandler> terminationHandler);

  // Clone the Game Process
  std::shared_ptr<TurnBasedGameProcess> clone();

  void seedRandomGenerator(uint32_t seed) override;

 private:
  static const std::string name_;
};
}  // namespace griddly