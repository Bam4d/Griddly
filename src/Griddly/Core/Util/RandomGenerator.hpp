#include <random>

namespace griddly {

    class RandomGenerator {
    public:
        RandomGenerator(size_t seed = 0) : randomGenerator_(seed) {}

        virtual ~RandomGenerator() = default;

        virtual void seed(int32_t seed);

        virtual int32_t sampleInt(int32_t min, int32_t max);

        virtual float sampleFloat(float min, float max);

        virtual std::mt19937 &getEngine();

    private:
        // Random number generator for the grid and associated objects
        std::mt19937 randomGenerator_;
    };

}  // namespace griddly
