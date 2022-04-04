#include "ObjectVariable.hpp"

#include <spdlog/spdlog.h>

#include "../Actions/Action.hpp"
#include "Object.hpp"

namespace griddly {

ObjectVariable::ObjectVariable(YAML::Node commandArguments, std::unordered_map<std::string, std::shared_ptr<int32_t>>& availableVariables, bool allowStrings) {
  auto commandArgumentValue = commandArguments.as<std::string>();

  auto delim = commandArgumentValue.find(".");
  if (delim != std::string::npos) {
    auto actionObjectStr = commandArgumentValue.substr(0, delim);
    if (actionObjectStr == "src") {
      actionObject_ = ActionObject::SRC;
    } else if (actionObjectStr == "dst") {
      actionObject_ = ActionObject::DST;
    } else if (actionObjectStr == "meta") {
      actionObject_ = ActionObject::META;
    } else {
      auto error = fmt::format("Unresolvable qualifier={0}, must be 'src', 'dst' or 'meta'", actionObjectStr);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }

    objectVariableType_ = ObjectVariableType::UNRESOLVED;
    variableName_ = commandArgumentValue.substr(delim + 1);
  } else {
    auto variable = availableVariables.find(commandArgumentValue);

    if (variable == availableVariables.end()) {
      spdlog::debug("Variable string not found, trying to parse literal={0}", commandArgumentValue);

      try {
        objectVariableType_ = ObjectVariableType::LITERAL;
        literalValue_ = std::stoi(commandArgumentValue);
        spdlog::debug("Literal value {0} resolved.", literalValue_);
      } catch (const std::exception& e) {
        if(allowStrings) {
          objectVariableType_ = ObjectVariableType::STRING;
          stringValue_ = commandArgumentValue;
        } else {
          auto error = fmt::format("Undefined variable={0}", commandArgumentValue);
          spdlog::error(error);
          throw std::invalid_argument(error);
        }
      }
    } else {
      spdlog::debug("Variable pointer {0} resolved.", variable->first);
      objectVariableType_ = ObjectVariableType::RESOLVED;
      resolvedValue_ = variable->second;
    }
  }
}

int32_t ObjectVariable::resolve(std::shared_ptr<Action> action) const {
  int32_t resolved = 0;
  switch (objectVariableType_) {
    case ObjectVariableType::STRING: {
      auto error = fmt::format("Variable is a string. Value cannot be resolved.", variableName_);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }
    case ObjectVariableType::LITERAL:
      resolved = literalValue_;
      spdlog::debug("resolved literal {0}", resolved);
      break;
    default:
      resolved = *resolve_ptr(action);
      spdlog::debug("resolved pointer value {0}", resolved);
      break;
  }

  return resolved;
}  // namespace griddly

std::string ObjectVariable::resolveString(std::shared_ptr<Action> action) const {
  if(objectVariableType_ == ObjectVariableType::STRING) {
    return stringValue_;
  } else {
    return std::to_string(resolve(action));
  }
}

std::shared_ptr<int32_t> ObjectVariable::resolve_ptr(std::shared_ptr<Action> action) const {
  switch (objectVariableType_) {
    case ObjectVariableType::STRING: {
      auto error = fmt::format("Variable is a string. Value cannot be resolved.", variableName_);
      spdlog::error(error);
      throw std::invalid_argument(error);
    }
    case ObjectVariableType::RESOLVED:
      return resolvedValue_;
    case ObjectVariableType::UNRESOLVED: {
      std::shared_ptr<int32_t> ptr;
      switch (actionObject_) {
        case ActionObject::SRC: {
          auto object = action->getSourceObject();
          ptr = object->getVariableValue(variableName_);
        } break;
        case ActionObject::DST: {
          auto object = action->getDestinationObject();
          ptr = object->getVariableValue(variableName_);
        } break;
        case ActionObject::META: {
          ptr = std::make_shared<int32_t>(action->getMetaData(variableName_));
          break;
        }
      }
      if (ptr == nullptr) {
        auto error = fmt::format("Undefined variable={0}", variableName_);
        throw std::invalid_argument(error);
      }
      return ptr;
    }
    default:
      throw std::runtime_error("Unresolvable variable!");
  }
}

}  // namespace griddly