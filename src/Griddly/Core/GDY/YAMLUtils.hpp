#pragma once

#include <vector>

namespace YAML {
class Node;
}

#define BehaviourCommandArguments std::unordered_map<std::string, YAML::Node>

namespace griddly {

inline std::vector<std::string> singleOrListNodeToList(YAML::Node singleOrList) {
  std::vector<std::string> values;
  if (singleOrList.IsScalar()) {
    values.push_back(singleOrList.as<std::string>());
  } else if (singleOrList.IsSequence()) {
    for (std::size_t s = 0; s < singleOrList.size(); s++) {
      values.push_back(singleOrList[s].as<std::string>());
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
  }

  return map;
}

}