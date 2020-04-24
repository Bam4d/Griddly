#include <spdlog.h>
#include <yaml.h>

#include "GDYParser.hpp"
#include "Objects/Object.hpp"

namespace griddy {
namespace gdy {

GDYParser::GDYParser(std::istream& yamlStringStream) {
  parse(yamlStringStream);
}

GDYParser::~GDYParser() {
}

std::shared_ptr<GameProcess> GDLParser::parse(std::istream& yamlStringStream) {
  auto gdyConfig = YAML::Load(yamlStringStream);

  auto environment = gdyConfig["environment"];
  auto objects = gdyConfig["objects"];
  auto actions = gdyConfig["actions"];

  loadObjects(objects);
  loadActions(actions);

  return loadEnvironment(environment);
}

void GDYParser::loadEnvironment(YAML::Node environment) {
}

void GDYParser::loadObjects(YAML::Node objects) {
  // Loop over the objects in the yaml and generate them
  // we need a global object cache which maps the object name to the object
  // we need a global action cache which maps the action name to the actions
  // Objects need reference to action names and object names?

  for (std::size_t i = 0; i < objects.size(); i++) {
    auto object = objects[i];
    auto name = object["Name"].as<std::string>();
    auto sprite = object["Sprite"].as<std::string>();
    auto params = object["Parameters"];

    std::unordered_map<std::string, std::shared_ptr<float>> parameters;

    for (std::size_t p = 0; p < params.size(); p++) {
      auto param = params[p];
      auto paramName = param['Name'].as<std::string>();
      auto paramInitialValue = param['InitialValue'].as<float>();

      parameters.insert({paramName, paramInitialValue});
    }

    spriteData_.insert({name, sprite});

    auto initializedObject = std::shared_ptr<Object>(new Object(name, parameters));
    objectData_.insert({name, initializedObject});
  }
}

void GDYParser::loadActions(YAML::Node actions) {

  for (std::size_t i = 0; i < actions.size(); i++) {
    auto action = actions[i];
    auto name = action["Name"].as<std::string>();
    auto behaviours = action["Behaviours"];

    for (std::size_t b = 0; b < behaviours.size(); b++) {
      auto behaviour = behaviours[b];
      auto src = behaviour["Src"];
      auto dst = behaviour["Dst"]
    }

  actionData_
}
}  // namespace gdy
}  // namespace griddy