#include "gtest/gtest.h"
#include "Griddly/Core/Util/RandomGenerator.hpp"

const int32_t EXPECTED_INT_SAMPLE = 5334164;
const float EXPECTED_FLOAT_SAMPLE = 432876.375;


TEST(RandomNumberGeneration, uniform_int_reproducibility) {
    auto rng = griddly::RandomGenerator(0);
    const int32_t upper_limit = 10'000'000;
    const size_t advance_by_n = 100'100;
    for (size_t i = 0; i < advance_by_n; i++) {
        rng.sampleInt(0, upper_limit);
    }
    auto sample = rng.sampleInt(0, upper_limit);
    ASSERT_EQ(sample, EXPECTED_INT_SAMPLE);
}

TEST(RandomNumberGeneration, uniform_real_reproducibility) {
    auto rng = griddly::RandomGenerator(0);
    const float upper_limit = 1'000'000.;
    const size_t advance_by_n = 100'100;
    for (size_t i = 0; i < advance_by_n; i++) {
        rng.sampleFloat(0, upper_limit);
    }
    auto sample = rng.sampleFloat(0, upper_limit);
    ASSERT_EQ(sample, EXPECTED_FLOAT_SAMPLE);
}