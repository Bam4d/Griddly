#pragma once
#include <memory>
#include <vector>

#include "GameProcess.hpp"
#include "Util/util.hpp"
#include "GameState.hpp"

namespace griddly {

class GDYFactory;

class TurnBasedGameProcess : public GameProcess {
 public:
  TurnBasedGameProcess(std::string globalObserverName, std::shared_ptr<GDYFactory> gdyFactory, std::shared_ptr<Grid> grid);
  ~TurnBasedGameProcess() override;

  ActionResult performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions, bool updateTicks = true) override;

  // Pooled variant: actions come from grid->acquireAction (zero per-tick allocs).
  ActionResult performActionsPooled(uint32_t playerId, const std::vector<Action*>& actions, bool updateTicks = true);

  std::string getProcessName() const override;

  void setTerminationHandler(std::shared_ptr<TerminationHandler> terminationHandler);

  // Clone the Game Process
  std::shared_ptr<TurnBasedGameProcess> clone();

  // Fast clone: memcpy the ECS state block into a new grid and share the
  // ObjectRules graph (no per-object cloneInstance/addObject).
  std::shared_ptr<TurnBasedGameProcess> cloneFromArena();

  // GameState API
  std::shared_ptr<TurnBasedGameProcess> fromGameState(GameState& gameState);

  void seedRandomGenerator(uint32_t seed) override;

 private:
  static const std::string name_;
};
}  // namespace griddly