#include "Player.hpp"
#include "StepObservation.hpp"


namespace griddy {

class StepPlayer : public Player {
  public:
    StepPlayer(int id, std::string playerName, std::shared_ptr<GameProcess> gameProcess);
    ~StepPlayer();
    StepObservation step(std::vector<std::shared_ptr<Action>> actions);

  private:
};
}