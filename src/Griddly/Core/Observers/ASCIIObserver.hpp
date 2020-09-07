#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddly {

class ASCIIObserver : public Observer {
public:
	ASCIIObserver(std::shared_ptr<Grid> grid);
	~ASCIIObserver() override;

	void init(ObserverConfig observerConfig) override;

	std::shared_ptr<uint8_t> update(int playerId) const override;
	std::shared_ptr<uint8_t> reset() override;

	void print(std::shared_ptr<uint8_t> observation);

private:
};

}  // namespace griddly