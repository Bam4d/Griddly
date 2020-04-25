#pragma once
#include <memory>
#include <sstream>

#include "../GameProcess.hpp"

namespace YAML {
class Node;
}

namespace griddy {
class GDYParser {
 public:
  GDYParser(std::istream& yamlString);
  ~GDYParser();

  std::shared_ptr<GameProcess> build();

 private:
  void parse(std::istream yamlString);

  void loadEnvironment(YAML::Node environment);

  void loadObjects(YAML::Node objects);

  void loadActions(YAML::Node actions);

  std::unordered_map<std::string, std::string> spriteData_;
  std::unordered_map<std::string, std::shared_ptr<Object>> objectData_;
  std::unordered_map<std::string, std::shared_ptr<Action>> actionData_;
};
}  // namespace griddy