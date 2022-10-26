#include <random>

namespace griddly {

class RandomGenerator {
 public:
  virtual void seed(int32_t seed);

  virtual const int32_t sampleInt(int32_t min, int32_t max);

  virtual const float sampleFloat(float min, float max);

  virtual std::mt19937& getEngine();

 private:
  // Random number generator for the grid and associated objects
  std::mt19937 randomGenerator_ = std::mt19937();
};

}  // namespace griddly
