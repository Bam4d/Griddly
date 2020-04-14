#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddy {

class TileObserver : public Observer {
 public:
  TileObserver(std::shared_ptr<Grid> grid);
  ~TileObserver() override;

  std::unique_ptr<uint8_t[]> observe(int playerId) const override;

  std::vector<uint> getShape() const override;
  std::vector<uint> getStrides() const override;

  void print(std::unique_ptr<uint8_t[]> observation) override;

 private:
};

}  // namespace griddy