#include "jlcxx/jlcxx.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <memory>

// Griddly module

#include "../../src/Griddly/Core/GDY/GDYFactory.hpp"
#include "../../src/Griddly/Core/GDY/Objects/ObjectGenerator.hpp"
#include "../../src/Griddly/Core/GDY/TerminationGenerator.hpp"
#include "../../src/Griddly/Core/Grid.hpp"


using namespace griddly;

namespace jlcxx{

} // namespace jlcxx


/*#ifndef NDEBUG
  	spdlog::set_level(spdlog::level::debug);
#else
	spdlog::set_level(spdlog::level::info);
#endif

		spdlog::debug("Julia Griddly module loaded!");*/

JULIA_CPP_MODULE_BEGIN(registry)
jlcxx::Module &mod = registry.create_module("JuGriddly");

/* GDYFactory */
/*----------------------------------------------------------------------------------------------------------------*/
mod.add_type<GDYFactory>("GDYFactory")
	.method("load_level", &GDYFactory::loadLevel)
	.method("load_level_string", &GDYFactory::loadLevelString)
	.method("create_level", &GDYFactory::createLevel)
	.method("load_environment", &GDYFactory::loadEnvironment);







