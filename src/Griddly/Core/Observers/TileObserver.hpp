#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddly {

class TileObserver : public Observer {
 public:
  TileObserver(std::shared_ptr<Grid> grid);
  ~TileObserver() override;

  std::shared_ptr<uint8_t> update(int playerId) const override;
  std::shared_ptr<uint8_t> reset() const override;

  std::vector<uint32_t> getShape() const override;
  std::vector<uint32_t> getStrides() const override;

  void print(std::shared_ptr<uint8_t> observation) override;

 private:
};

}  // namespace griddly