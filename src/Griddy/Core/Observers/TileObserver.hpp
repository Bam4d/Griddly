#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddy {

class TileObserver : public Observer {
 public:
  TileObserver();
  ~TileObserver() override;

  std::shared_ptr<uint8_t[]> observe(int playerId, std::shared_ptr<Grid> grid) override;

  void print(std::shared_ptr<uint8_t[]> observation, std::shared_ptr<Grid> grid) override;

 private:
};

}  // namespace griddy