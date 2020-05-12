#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace griddy {

class Grid;
class TerminationHandler;


class TerminationGenerator {
 public:
  virtual void defineTerminationCondition(std::string commandName, std::vector<std::string> commandParameters);
  virtual std::shared_ptr<TerminationHandler> newInstance(std::shared_ptr<Grid> grid);

 private:
  std::vector<std::pair<std::string, std::vector<std::string>>> terminationConditionDefinitions_;
};
}  // namespace griddy