#include "ObjectVariable.hpp"

#include <spdlog/spdlog.h>

#include "../Actions/Action.hpp"
#include "Object.hpp"

namespace griddly {

ObjectVariable::ObjectVariable(YAML::Node commandArguments, std::unordered_map<std::string, std::shared_ptr<int32_t>>& availableVariables) {
  auto commandArgumentValue = commandArguments.as<std::string>();

  auto delim = commandArgumentValue.find(".");
  if (delim != std::string::npos) {
    auto actionObjectStr = commandArgumentValue.substr(0, delim);
    if (actionObjectStr == "src") {
      actionObject_ = ActionObject::SRC;
    } else if (actionObjectStr == "dst") {
      actionObject_ = ActionObject::DST;
    } else {
      auto error = fmt::format("Unresolvable qualifier={0}, must be 'src' or 'dst'", actionObjectStr);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    objectVariableType_ = ObjectVariableType::UNRESOLVED;
    variableName_ = commandArgumentValue.substr(delim+1);
  } else {
    auto variable = availableVariables.find(commandArgumentValue);
    std::shared_ptr<int32_t> resolvedVariable;

    if (variable == availableVariables.end()) {
      spdlog::debug("Variable string not found, trying to parse literal={0}", commandArgumentValue);

      try {
        objectVariableType_ = ObjectVariableType::LITERAL;
        literalValue_ = std::stoi(commandArgumentValue);
      } catch (const std::exception& e) {
        auto error = fmt::format("Undefined variable={0}", commandArgumentValue);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
    } else {
      objectVariableType_ = ObjectVariableType::RESOLVED;
      resolvedValue_ = variable->second;
    }
  }
}

int32_t ObjectVariable::resolve(std::shared_ptr<Action> action) const {
  switch (objectVariableType_) {
    case ObjectVariableType::LITERAL:
      return literalValue_;
    case ObjectVariableType::RESOLVED:
      return *resolvedValue_;
    case ObjectVariableType::UNRESOLVED: {
      std::shared_ptr<Object> object;
      switch (actionObject_) {
        case ActionObject::SRC:
          object = action->getSourceObject();
          break;
        case ActionObject::DST:
          object = action->getDestinationObject();
      }
      return *object->getVariableValue(variableName_);
    }
    default:
      throw std::runtime_error("Unresolvable variable!");
  }
}

}  // namespace griddly