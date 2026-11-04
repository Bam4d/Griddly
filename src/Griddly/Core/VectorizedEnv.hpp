#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "GDY/GDYFactory.hpp"
#include "Players/Player.hpp"
#include "TurnBasedGameProcess.hpp"

namespace griddly {

struct VectorizedStepResult {
  std::vector<int32_t> rewards;    // per env
  std::vector<bool> terminated;    // per env (env was auto-reset)
  std::vector<bool> wins;          // per env, if terminated
};

// A pool of N independent game environments. Each env owns its own Grid and
// ECS arena (created via cloneFromArena, i.e. one memcpy), so stepping the
// envs is embarrassingly parallel: step() distributes envs over a thread
// pool and each worker calls performActionsPooled on its own env with zero
// shared mutable state. The compiled GDY rules (GDYFactory/ObjectRules) are
// shared read-only across all threads.
class VectorizedEnv {
 public:
  // numThreads = 0 -> hardware_concurrency
  VectorizedEnv(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs, uint32_t numThreads = 0);
  ~VectorizedEnv();

  VectorizedEnv(const VectorizedEnv&) = delete;
  VectorizedEnv& operator=(const VectorizedEnv&) = delete;

  uint32_t numEnvs() const { return numEnvs_; }
  uint32_t numThreads() const { return (uint32_t)pool_.size(); }

  void loadLevel(uint32_t levelId);
  void loadLevelString(const std::string& levelString);
  void init();
  void reset();  // full reset (regenerates level) on all envs, in parallel

  // actions[i] = list of action arrays (int32, external action input format)
  // for env i, player 1. Envs are stepped in parallel on the thread pool.
  // Envs that terminate are auto-reset (one memcpy) and reported in the
  // result.
  VectorizedStepResult step(const std::vector<std::vector<std::vector<int32_t>>>& actions);

  // Seed env i's RNG with seed+i so envs are independent.
  void seed(uint32_t seed);

  std::shared_ptr<TurnBasedGameProcess> env(uint32_t i) const { return envs_.at(i); }
  std::shared_ptr<Grid> grid(uint32_t i) const { return envs_.at(i)->getGrid(); }

 private:
  void workerLoop();
  enum class Job;
  void runParallel(Job job, const std::vector<std::vector<std::vector<int32_t>>>& perEnvWork);
  void stepEnv(uint32_t i, const std::vector<std::vector<int32_t>>& actionArrays);
  void resetEnv(uint32_t i);
  void loadLevelEnv(uint32_t i, uint32_t levelId);

  Action* buildAction(uint32_t envIdx, const std::string& actionName, const std::vector<int32_t>& actionArray);

  std::shared_ptr<GDYFactory> gdyFactory_;
  uint32_t numEnvs_;
  std::string observerName_ = "None";

  std::vector<std::shared_ptr<TurnBasedGameProcess>> envs_;
  std::vector<std::shared_ptr<Player>> players_;

  // results slots (written by workers, read by caller after join)
  VectorizedStepResult result_;

  // minimal thread pool with index-based work distribution
  std::vector<std::thread> pool_;
  std::mutex mutex_;
  std::condition_variable cvWork_, cvDone_;
  // job state
  const std::vector<std::vector<std::vector<int32_t>>>* work_ = nullptr;
  enum class Job { NONE, STEP, RESET, LOAD_LEVEL };
  Job job_ = Job::NONE;
  uint32_t nextIdx_ = 0;
  uint32_t remaining_ = 0;
  uint32_t levelId_ = 0;
  bool initialized_ = false;
  bool stop_ = false;
};

}  // namespace griddly
