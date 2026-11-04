#include "VectorizedEnv.hpp"

#include <algorithm>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "GDY/Actions/Action.hpp"

namespace griddly {

VectorizedEnv::VectorizedEnv(std::shared_ptr<GDYFactory> gdyFactory, uint32_t numEnvs, uint32_t numThreads)
    : gdyFactory_(std::move(gdyFactory)), numEnvs_(numEnvs) {
  if (numEnvs_ == 0) {
    throw std::invalid_argument("VectorizedEnv requires at least 1 env");
  }

  uint32_t nThreads = numThreads == 0 ? std::thread::hardware_concurrency() : numThreads;
  if (nThreads == 0) nThreads = 1;
  nThreads = std::min(nThreads, numEnvs_);

  // env 0 is the template; the rest are arena clones of it after init
  auto grid = std::make_shared<Grid>(Grid());
  envs_.push_back(std::make_shared<TurnBasedGameProcess>(TurnBasedGameProcess(observerName_, gdyFactory_, grid)));
  auto player = std::make_shared<Player>(Player(1, "Player 1", observerName_, envs_[0]));
  envs_[0]->addPlayer(player);
  players_.push_back(player);

  result_.rewards.resize(numEnvs_);
  result_.terminated.resize(numEnvs_);
  result_.wins.resize(numEnvs_);

  for (uint32_t t = 0; t < nThreads; t++) {
    pool_.emplace_back([this] { workerLoop(); });
  }
}

VectorizedEnv::~VectorizedEnv() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stop_ = true;
  }
  cvWork_.notify_all();
  for (auto& t : pool_) {
    if (t.joinable()) t.join();
  }
}

void VectorizedEnv::init() {
  if (initialized_) return;
  envs_[0]->init(false);
  initialized_ = true;
  // clone the initialized template into envs 1..N-1 (one arena memcpy each)
  for (uint32_t i = 1; i < numEnvs_; i++) {
    auto cloned = envs_[0]->cloneFromArena();
    auto player = std::make_shared<Player>(Player(1, "Player 1", observerName_, cloned));
    cloned->addPlayer(player);
    cloned->init(true);
    envs_.push_back(cloned);
    players_.push_back(player);
  }
}

void VectorizedEnv::loadLevel(uint32_t levelId) {
  if (!initialized_) init();
  levelId_ = levelId;
  runParallel(Job::LOAD_LEVEL, {});
}

void VectorizedEnv::loadLevelEnv(uint32_t i, uint32_t levelId) {
  envs_[i]->setLevel(levelId);
  envs_[i]->reset();
}

void VectorizedEnv::loadLevelString(const std::string& levelString) {
  for (auto& e : envs_) {
    e->setLevel(levelString);
    e->reset();
  }
}

void VectorizedEnv::reset() {
  runParallel(Job::RESET, {});
}

void VectorizedEnv::seed(uint32_t seed) {
  for (uint32_t i = 0; i < numEnvs_; i++) {
    envs_[i]->seedRandomGenerator(seed + i);
  }
}

VectorizedStepResult VectorizedEnv::step(const std::vector<std::vector<std::vector<int32_t>>>& actions) {
  if (actions.size() != numEnvs_) {
    throw std::invalid_argument("VectorizedEnv::step: actions.size() must equal numEnvs");
  }
  result_.rewards.assign(numEnvs_, 0);
  result_.terminated.assign(numEnvs_, false);
  result_.wins.assign(numEnvs_, false);
  runParallel(Job::STEP, actions);
  return result_;
}

void VectorizedEnv::runParallel(Job job, const std::vector<std::vector<std::vector<int32_t>>>& perEnvWork) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    work_ = &perEnvWork;
    job_ = job;
    nextIdx_ = 0;
    remaining_ = numEnvs_;
  }
  cvWork_.notify_all();
  std::unique_lock<std::mutex> lock(mutex_);
  cvDone_.wait(lock, [this] { return remaining_ == 0; });
}

void VectorizedEnv::workerLoop() {
  for (;;) {
    std::unique_lock<std::mutex> lock(mutex_);
    cvWork_.wait(lock, [this] { return stop_ || (job_ != Job::NONE && nextIdx_ < numEnvs_); });
    if (stop_) return;
    if (nextIdx_ >= numEnvs_) {
      if (remaining_ == 0) cvDone_.notify_one();
      continue;
    }
    uint32_t i = nextIdx_++;
    Job job = job_;
    lock.unlock();

    switch (job) {
      case Job::STEP:
        stepEnv(i, (*work_)[i]);
        break;
      case Job::RESET:
        resetEnv(i);
        break;
      case Job::LOAD_LEVEL:
        loadLevelEnv(i, levelId_);
        break;
      default:
        break;
    }

    lock.lock();
    if (--remaining_ == 0) {
      job_ = Job::NONE;
      cvDone_.notify_one();
    }
  }
}

void VectorizedEnv::stepEnv(uint32_t i, const std::vector<std::vector<int32_t>>& actionArrays) {
  auto& gp = envs_[i];
  auto grid = gp->getGrid();

  std::vector<Action*> actions;
  actions.reserve(actionArrays.size());

  const auto& externalActionNames = gdyFactory_->getExternalActionNames();

  for (const auto& raw : actionArrays) {
    std::string actionName;
    std::vector<int32_t> actionArray;
    switch (raw.size()) {
      case 1:
        actionName = externalActionNames.at(0);
        actionArray.push_back(raw[0]);
        break;
      case 2:
        actionName = externalActionNames.at(raw[0]);
        actionArray.push_back(raw[1]);
        break;
      case 3:
        actionName = externalActionNames.at(0);
        actionArray = raw;
        break;
      case 4:
        actionName = externalActionNames.at(raw[2]);
        actionArray.push_back(raw[0]);
        actionArray.push_back(raw[1]);
        actionArray.push_back(raw[3]);
        break;
      default:
        throw std::invalid_argument("VectorizedEnv: invalid action size");
    }
    auto* action = buildAction(i, actionName, actionArray);
    if (action != nullptr) actions.push_back(action);
  }

  auto res = gp->performActionsPooled(1, actions, true);

  result_.rewards[i] = gp->getAccumulatedRewards(1);
  result_.terminated[i] = res.terminated;
  if (res.terminated) {
    auto it = res.playerStates.find(1);
    result_.wins[i] = it != res.playerStates.end() && it->second == TerminationState::WIN;
    // auto-reset terminated envs with a single arena memcpy
    gp->resetFromSnapshot();
  }
}

void VectorizedEnv::resetEnv(uint32_t i) {
  envs_[i]->reset();
}

Action* VectorizedEnv::buildAction(uint32_t envIdx, const std::string& actionName, const std::vector<int32_t>& actionArray) {
  const auto& def = gdyFactory_->findActionInputsDefinition(actionName);
  const auto& inputMappings = def.inputMappings;
  auto grid = envs_[envIdx]->getGrid();
  auto avatar = players_[envIdx]->getAvatar();

  if (avatar != nullptr) {
    auto actionId = actionArray[0];
    if (inputMappings.find(actionId) == inputMappings.end()) {
      return nullptr;
    }
    const auto& mapping = inputMappings.at(actionId);
    auto* action = grid->acquireAction(actionName, 1);
    action->init(avatar, mapping.vectorToDest, mapping.orientationVector, def.relative);
    return action;
  } else {
    glm::ivec2 sourceLocation = {actionArray[0], actionArray[1]};
    auto actionId = actionArray[2];
    if (inputMappings.find(actionId) == inputMappings.end()) {
      return nullptr;
    }
    const auto& mapping = inputMappings.at(actionId);
    glm::ivec2 destinationLocation = sourceLocation + mapping.vectorToDest;
    auto* action = grid->acquireAction(actionName, 1);
    action->init(sourceLocation, destinationLocation);
    return action;
  }
}

}  // namespace griddly
