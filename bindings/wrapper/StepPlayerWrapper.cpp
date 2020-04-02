#include <memory>
#include "../../src/Griddy/Core/Players/StepPlayer.hpp"
#include "../../src/Griddy/Core/Actions/ActionTypes.hpp"
#include "../../src/Griddy/Core/Actions/Gather.hpp"
#include "../../src/Griddy/Core/Actions/Move.hpp"
#include "../../src/Griddy/Core/Actions/Punch.hpp"

namespace griddy {
class Py_PlayerWrapper {
 public:
  Py_PlayerWrapper(int playerId, std::string playerName) : player_(std::shared_ptr<StepPlayer>(new StepPlayer(playerId, playerName))) {
  }

  std::shared_ptr<StepPlayer> unwrapped() {
    return player_;
  }

  int step(int x, int y, ActionType actionType, Direction direction) {

    auto gameProcess = player_->getGameProcess();
    
    if (gameProcess != nullptr && gameProcess->isStarted()) {
        throw std::invalid_argument("Cannot send player commands when game has no been started. Please build the environment using grid.start_env()");
    }

    std::shared_ptr<Action> action;
    switch(actionType) {
      case MOVE:
        action = std::shared_ptr<Action>(new Move(direction, {x, y}));
      break;
      case GATHER:
        action = std::shared_ptr<Action>(new Gather(direction, {x, y}));
      break;
      case PUNCH:
        action = std::shared_ptr<Action>(new Punch(direction, {x, y}));
      break;
    }

    auto step_observation = player_->step({action});

    return step_observation.reward;
  }

 private:
  const std::shared_ptr<StepPlayer> player_;
};
}  // namespace griddy