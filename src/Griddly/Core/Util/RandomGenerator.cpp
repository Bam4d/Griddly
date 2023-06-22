#include "RandomGenerator.hpp"

#include "boost/random.hpp"


namespace griddly {


void RandomGenerator::seed(int32_t seed) {
  randomGenerator_.seed(seed);
}

int32_t RandomGenerator::sampleInt(int32_t min, int32_t max) {
  boost::uniform_int<int32_t> dist(min, max);
  return dist(randomGenerator_);
}

float RandomGenerator::sampleFloat(float min, float max) {
    boost::uniform_real<float> dist(min, max);
  return dist(randomGenerator_);
}

std::mt19937& RandomGenerator::getEngine() {
  return randomGenerator_;
}


}  // namespace griddly