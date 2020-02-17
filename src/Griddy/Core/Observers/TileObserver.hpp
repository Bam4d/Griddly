#include "Observer.hpp"
#include "../Grid.hpp"

namespace griddy {

class TileObserver : public Observer {
 public:
  TileObserver();
  ~TileObserver();

  std::unique_ptr<uint8_t[]> observe(int playerId, std::shared_ptr<Grid> grid) override;

  void print(std::unique_ptr<uint8_t[]> observation,
           std::shared_ptr<Grid> grid) override;

 private:
};

}  // namespace griddy