#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "Griddly/Core/GDY/GDYFactory.hpp"
#include "Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "Griddly/Core/GDY/TerminationGenerator.hpp"
#include "Griddly/Core/VectorizedEnv.hpp"

namespace griddly {

class VectorizedEnvTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto objectGenerator = std::make_shared<ObjectGenerator>();
    auto terminationGenerator = std::make_shared<TerminationGenerator>();
    gdyFactory_ = std::make_shared<GDYFactory>(objectGenerator, terminationGenerator, ResourceConfig{});
    gdyFactory_->initializeFromFile("tests/resources/ecs_test.yaml");
  }

  // fixed pseudo-random action sequence, deterministic across runs
  // seq[step][env] = list of action arrays for that env
  std::vector<std::vector<std::vector<std::vector<int32_t>>>> makeActions(uint32_t numEnvs, uint32_t steps) {
    std::vector<std::vector<std::vector<std::vector<int32_t>>>> seq(
        steps, std::vector<std::vector<std::vector<int32_t>>>(numEnvs, {{1}}));
    uint32_t x = 12345;
    for (auto& step : seq) {
      for (auto& env : step) {
        x = x * 1664525 + 1013904223;
        env[0][0] = static_cast<int32_t>(((x >> 16) % 2) + 1);  // action id 1 or 2
      }
    }
    return seq;
  }

  std::shared_ptr<GDYFactory> gdyFactory_;
};

TEST_F(VectorizedEnvTest, ParallelMatchesSingleEnv) {
  const uint32_t numEnvs = 8;
  const uint32_t steps = 200;

  VectorizedEnv single(gdyFactory_, 1, 1);
  single.loadLevel(0);
  single.seed(777);

  VectorizedEnv multi(gdyFactory_, numEnvs, 4);
  multi.loadLevel(0);
  multi.seed(777);

  auto actions = makeActions(numEnvs, steps);

  std::vector<int32_t> singleRewards;
  std::vector<int32_t> env0Rewards;
  for (uint32_t s = 0; s < steps; s++) {
    auto r1 = single.step(std::vector<std::vector<std::vector<int32_t>>>{actions[s][0]});
    (void)r1;
    singleRewards.push_back(r1.rewards[0]);
    auto rN = multi.step(actions[s]);
    env0Rewards.push_back(rN.rewards[0]);
    // env 0 of the pool is seeded identically (seed + 0) and receives the
    // same actions, so its trajectory must match the single env exactly
    ASSERT_EQ(r1.rewards[0], rN.rewards[0]) << "reward mismatch at step " << s;
    ASSERT_EQ(r1.terminated[0], rN.terminated[0]) << "terminated mismatch at step " << s;
  }
}

TEST_F(VectorizedEnvTest, ResetRestoresInitialState) {
  VectorizedEnv env(gdyFactory_, 4, 2);
  env.loadLevel(0);
  env.seed(99);

  auto actions = makeActions(4, 50);
  for (uint32_t s = 0; s < 50; s++) env.step(actions[s]);

  env.reset();

  // after reset, same seed + same actions must reproduce the same rewards
  env.seed(99);
  VectorizedEnv fresh(gdyFactory_, 4, 2);
  fresh.loadLevel(0);
  fresh.seed(99);

  for (uint32_t s = 0; s < 50; s++) {
    auto r1 = env.step(actions[s]);
    auto r2 = fresh.step(actions[s]);
    for (uint32_t e = 0; e < 4; e++) {
      ASSERT_EQ(r1.rewards[e], r2.rewards[e]) << "env " << e << " step " << s;
    }
  }
}

TEST_F(VectorizedEnvTest, StepRejectsWrongActionCount) {
  VectorizedEnv env(gdyFactory_, 4, 2);
  env.loadLevel(0);
  EXPECT_THROW(env.step({{{1}}, {{1}}}), std::invalid_argument);
}

}  // namespace griddly
