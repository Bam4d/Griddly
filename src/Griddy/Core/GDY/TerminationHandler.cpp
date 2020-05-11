#include "TerminationHandler.hpp"

#include <spdlog/spdlog.h>

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>

namespace griddy {

void registerObject(std::string objectName) {

}

void defineTerminationCondition(TerminationConditionDefinition terminationConditionDefintion) {

}

std::vector<std::shared_ptr<int32_t>> TerminationHandler::findParameters(std::vector<std::string> parameters) {
  std::vector<std::shared_ptr<int32_t>> resolvedParams;
  for (auto &param : parameters) {
    auto parameter = availableParameters_.find(param);
    std::shared_ptr<int32_t> resolvedParam;

    if (parameter == availableParameters_.end()) {
      spdlog::debug("Parameter string not found, trying to parse literal={0}", param);

      try {
        resolvedParam = std::make_shared<int32_t>(std::stoi(param));
      } catch (const std::exception &e) {
        auto error = fmt::format("Undefined parameter={0}", param);
        spdlog::error(error);
        throw std::invalid_argument(error);
      }
    } else {
      resolvedParam = parameter->second;
    }

    resolvedParams.push_back(resolvedParam);
  }

  return resolvedParams;
}

}  // namespace griddy