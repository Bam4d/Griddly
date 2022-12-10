#pragma once
#include <yaml-cpp/yaml.h>

#include <vector>

#define CommandArguments std::map<std::string, YAML::Node>

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

inline CommandArguments singleOrListNodeToCommandArguments(YAML::Node singleOrList) {
  CommandArguments map;
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

inline void throwParserError(const std::string& errorString) {
  spdlog::error(errorString);
  throw std::invalid_argument(errorString);
}

inline YAML::iterator validateCommandPairNode(YAML::Node& commandPairNodeList) {
  if (commandPairNodeList.size() > 1) {
    auto line = commandPairNodeList.Mark().line;
    auto errorString = fmt::format("Parse Error line {0}. Each command must be defined as a singleton list. E.g '- set: ...\n- reward: ...'. \n You may have a missing '-' before the command.", line);
    throwParserError(errorString);
  }

  return commandPairNodeList.begin();
}

inline glm::vec3 parseColorNode(YAML::Node colorNode) {
  if (colorNode.IsDefined()) {
    if (!colorNode.IsSequence() || colorNode.size() != 3) {
      auto error = fmt::format("Color node misconfigured, must contain 3 values but contains {0}.", colorNode.size());
      throwParserError(error);
    } else {
      return glm::vec3{
          colorNode[0].as<float>(0),
          colorNode[1].as<float>(0),
          colorNode[2].as<float>(0)};
    }
  } 

  return glm::vec3{};
}

}  // namespace griddly