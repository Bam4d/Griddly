#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddly {

class VectorObserver : public Observer {
 public:
  VectorObserver(std::shared_ptr<Grid> grid);
  ~VectorObserver() override;

  void init(ObserverConfig observerConfig) override;

  std::shared_ptr<uint8_t> update(int playerId) const override;
  std::shared_ptr<uint8_t> reset() override;

  void print(std::shared_ptr<uint8_t> observation) override;

 private:
};

}  // namespace griddly