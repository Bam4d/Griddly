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
  UNRESOLVED,
  STRING
};

enum class ActionObject {
  SRC,
  DST,
  META,
};

class ObjectVariable {
 public:
  ObjectVariable(const YAML::Node& commandArguments, const std::unordered_map<std::string, std::shared_ptr<int32_t>>& availableVariables, bool allowStrings=false);
  int32_t resolve(std::shared_ptr<Action> action) const;
  std::shared_ptr<int32_t> resolve_ptr(std::shared_ptr<Action> action) const;

  std::string resolveString(std::shared_ptr<Action> action) const;

 private:
  ObjectVariableType objectVariableType_;

  // Literal value
  int32_t literalValue_;

  // pre-resolved value
  std::shared_ptr<int32_t> resolvedValue_;

  // String value
  std::string stringValue_;

  // value that needs to be resolved at time of action
  std::string variableName_;
  ActionObject actionObject_;
};
}  // namespace griddly