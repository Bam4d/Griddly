#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddy {

class TileObserver : public Observer {
 public:
  TileObserver(std::shared_ptr<Grid> grid);
  ~TileObserver() override;

  std::unique_ptr<uint8_t[]> observe(int playerId) override;

  std::vector<size_t> getShape() const override;
  std::vector<size_t> getStrides() const override;

  void print(std::unique_ptr<uint8_t[]> observation) override;

 private:
};

}  // namespace griddy