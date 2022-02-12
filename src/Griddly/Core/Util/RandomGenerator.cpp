#include "RandomGenerator.hpp"

namespace griddly {


void RandomGenerator::seed(int32_t seed) {
  randomGenerator_.seed(seed);
}

const int32_t RandomGenerator::sampleInt(int32_t min, int32_t max) {
  std::uniform_int_distribution<int32_t> dist(min, max);
  return dist(randomGenerator_);
}

const float RandomGenerator::sampleFloat(float min, float max) {
  std::uniform_real_distribution<float> dist(min, max);
  return dist(randomGenerator_);
}

}  // namespace griddly