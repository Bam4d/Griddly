#pragma once
#include <yaml-cpp/yaml.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace griddly {

class Action;

enum class ObjectVariableType {
  LITERAL,
  RESOLVED,
  UNRESOLVED
};

enum class ActionObject {
  SRC,
  DST,
  META,
};

class ObjectVariable {
 public:
  ObjectVariable(YAML::Node commandArguments, std::unordered_map<std::string, std::shared_ptr<int32_t>>& availableVariables);
  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] int32_t resolve(std::shared_ptr<Action> action) const;
  [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] [[nodiscard]] std::shared_ptr<int32_t> resolve_ptr(std::shared_ptr<Action> action) const;

 private:
  ObjectVariableType objectVariableType_;

  // Literal value
  int32_t literalValue_;

  // pre-resolved value
  std::shared_ptr<int32_t> resolvedValue_;

  // value that needs to be resolved at time of action
  std::string variableName_;
  ActionObject actionObject_;
};
}  // namespace griddly