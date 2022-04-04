#pragma once
#include <yaml-cpp/yaml.h>

#include <vector>

#define BehaviourCommandArguments std::unordered_map<std::string, YAML::Node>

namespace griddly {

template <typename T = std::string>
inline std::vector<T> singleOrListNodeToList(YAML::Node singleOrList) {
  std::vector<T> values;
  if (singleOrList.IsScalar()) {
    values.push_back(singleOrList.as<T>());
  } else if (singleOrList.IsSequence()) {
    for (auto&& s : singleOrList) {
      values.push_back(s.as<T>());
    }
  }

  return values;
}

inline BehaviourCommandArguments singleOrListNodeToCommandArguments(YAML::Node singleOrList) {
  BehaviourCommandArguments map;
  if (singleOrList.IsScalar()) {
    map["0"] = singleOrList;
  } else if (singleOrList.IsSequence()) {
    for (std::size_t s = 0; s < singleOrList.size(); s++) {
      map[std::to_string(s)] = singleOrList[s];
    }
  } else if (singleOrList.IsMap()) {
    for (YAML::const_iterator mappingNode = singleOrList.begin(); mappingNode != singleOrList.end(); ++mappingNode) {
      map[mappingNode->first.as<std::string>()] = mappingNode->second;
    }
  }

  return map;
}

}  // namespace griddly