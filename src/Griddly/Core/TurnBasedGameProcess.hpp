#pragma once
#include <memory>
#include <vector>

#include "GameProcess.hpp"
#include "Util/util.hpp"

namespace griddly {

class GDYFactory;

class TurnBasedGameProcess : public GameProcess {
 public:
  TurnBasedGameProcess(ObserverType globalObserverType, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<Grid> grid);
  ~TurnBasedGameProcess();

  virtual ActionResult performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions, bool updateTicks=true) override;

  virtual std::string getProcessName() const override;

  void setTerminationHandler(std::shared_ptr<TerminationHandler> terminationHandler);

  int32_t getAccumulatedRewards(uint32_t playerId);

  // Clone the Game Process
  std::shared_ptr<TurnBasedGameProcess> clone();

 private:
  static const std::string name_;
  std::unordered_map<uint32_t, int32_t> accumulatedRewards_;
};
}  // namespace griddly