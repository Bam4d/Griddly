#pragma once
#include <yaml-cpp/yaml.h>

#include "../../ECS/ArenaPtr.hpp"

#include <memory>
#include <string>
#include <unordered_map>

namespace griddly {

class Action;
class Object;

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
  ObjectVariable(const YAML::Node& commandArguments, const VariableMap& availableVariables, const Object* owner = nullptr, bool allowStrings=false);
  int32_t resolve(std::shared_ptr<Action> action) const;
  IntVar resolve_ptr(std::shared_ptr<Action> action) const;

  std::string resolveString(std::shared_ptr<Action> action) const;

 private:
  ObjectVariableType objectVariableType_;

  // Literal value
  int32_t literalValue_;

  // pre-resolved value (arena handle or fallback)
  IntVar resolvedValue_;
  const Object* owner_ = nullptr;

  // String value
  std::string stringValue_;

  // value that needs to be resolved at time of action
  std::string variableName_;
  ActionObject actionObject_;
};
}  // namespace griddly