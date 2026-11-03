#pragma once

#include <array>
#include <spdlog/spdlog.h>

#include <iterator>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
inline void hash_combine(std::size_t& seed, const T& val) {
  seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename Out>
inline void split(const std::string& s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = item;
  }
}

inline std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

template <class T, class C = std::vector<T>, class P = std::less<typename C::value_type> >
struct VectorPriorityQueue : std::priority_queue<T, C, P> {
  using std::priority_queue<T, C, P>::priority_queue;
  typename C::const_iterator begin() const { return std::priority_queue<T, C, P>::c.begin(); }
  typename C::const_iterator end() const { return std::priority_queue<T, C, P>::c.end(); }
};

inline void accumulateRewards(std::unordered_map<uint32_t, int32_t>& acc, std::unordered_map<uint32_t, int32_t>& values) {
  for (auto valueIt : values) {
    acc[valueIt.first] += valueIt.second;
  }
}

// Fixed-capacity reward buffer (indexed by playerId) -- zero heap
// allocations in the per-action hot path; converted to the map form only
// at the public API boundary.
constexpr uint32_t MAX_REWARD_PLAYERS = 32;
using RewardArray = std::array<int32_t, MAX_REWARD_PLAYERS>;

inline void accumulateRewards(RewardArray& acc, const RewardArray& values) {
  for (uint32_t i = 0; i < MAX_REWARD_PLAYERS; ++i) acc[i] += values[i];
}

inline RewardArray rewardOf(uint32_t player, int32_t value) {
  RewardArray r{};
  if (player < MAX_REWARD_PLAYERS) r[player] = value;
  return r;
}

// Boundary conversions between the fixed hot-path buffer and the map form
// used by the public API.
inline std::unordered_map<uint32_t, int32_t> toRewardMap(const RewardArray& arr) {
  std::unordered_map<uint32_t, int32_t> m;
  for (uint32_t i = 0; i < MAX_REWARD_PLAYERS; ++i) {
    if (arr[i] != 0) m[i] = arr[i];
  }
  return m;
}

inline RewardArray rewardArrayOf(const std::unordered_map<uint32_t, int32_t>& m) {
  RewardArray r{};
  for (const auto& [player, value] : m) {
    if (player < MAX_REWARD_PLAYERS) r[player] = value;
  }
  return r;
}

inline void throwRuntimeError(const std::string& errorString) {
  spdlog::error(errorString);
  throw std::invalid_argument(errorString);
}

inline std::string generateRandomString(const int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return tmp_s;
}
