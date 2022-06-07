#include "Jiddly.hpp"

#include <spdlog/spdlog.h>

#include <iostream>

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddly/Core/GDY/TerminationGenerator.hpp"

Jiddly::Jiddly() {
#ifndef NDEBUG
  spdlog::set_level(spdlog::level::debug);
#else
  spdlog::set_level(spdlog::level::info);
#endif

  spdlog::debug("Jiddly module loaded!");
}

std::shared_ptr<JiddlyGDYWrapper> Jiddly::loadString(std::string levelString) {
  auto objectGenerator = std::make_shared<griddly::ObjectGenerator>(griddly::ObjectGenerator());
  auto terminationGenerator = std::make_shared<griddly::TerminationGenerator>(griddly::TerminationGenerator());
  auto gdyFactory = std::make_shared<griddly::GDYFactory>(griddly::GDYFactory(objectGenerator, terminationGenerator));
  std::istringstream s(levelString);
  gdyFactory->parseFromStream(s);
  return std::make_shared<JiddlyGDYWrapper>(JiddlyGDYWrapper(gdyFactory));
}