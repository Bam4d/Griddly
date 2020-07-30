#include "jlcxx/jlcxx.hpp"

#include <spdlog/spdlog.h>
#include <sstream>
#include <memory>

// Griddly module
#include "wrapper/NumpyWrapper.cpp"
#include "wrapper/JuGDYReaderWrapper.cpp"
#include "wrapper/JuGridWrapper.cpp"
#include "wrapper/JuStepPlayerWrapper.cpp"

using namespace griddly;

std::string greet()
{
	return "hello, world";
}

namespace jlcxx {


} // namespace jlcxx

JLCXX_MODULE define_module_jugriddly(jlcxx::Module& mod) {

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#else
	spdlog::set_level(spdlog::level::info);
#endif

	spdlog::debug("Julia Griddly module loaded!");

	/* Test config */
	mod.method("greet", &greet);

	/* GDYFactory */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<Ju_GDYReaderWrapper>("GDYReader")
		.method("load", &Ju_GDYReaderWrapper::loadGDYFile)
		.method("load_string", &Ju_GDYReaderWrapper::loadGDYString);

	/* Grid */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<Ju_GridWrapper>("JuGrid")
		.method("set_tile_size!", &Ju_GridWrapper::setTileSize)
		.method("get_tile_size", &Ju_GridWrapper::getTileSize)
		.method("get_width", &Ju_GridWrapper::getWidth)
		.method("get_height", &Ju_GridWrapper::getHeight)
		.method("get_player_count", &Ju_GridWrapper::getPlayerCount)
		/*.method("get_action_input_mappings", &Ju_GridWrapper::getActionInputMappings)*/
		.method("get_avatar_object", &Ju_GridWrapper::getAvatarObject)
		.method("create_level", &Ju_GridWrapper::createLevel)
		.method("load_level!", &Ju_GridWrapper::loadLevel)
		.method("load_level_string!", &Ju_GridWrapper::loadLevelString)
		.method("create_game", &Ju_GridWrapper::createGame)
		.method("add_object!", &Ju_GridWrapper::addObject)
		;

	/* StepPlayer */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<Ju_StepPlayerWrapper>("StepPlayerWrapper")
		.method("observe", &Ju_StepPlayerWrapper::observe)
		.method("step!", [](Ju_StepPlayerWrapper& jlstepplayer, std::string action_name, jlcxx::ArrayRef<int_t, 1> actions_array)
			{
				std::vector<int32_t> data(actions_array.begin(), actions_array.end());
				return jlstepplayer.step(action_name, data);
			});

	/* GameProcess */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<Ju_GameProcessWrapper>("JuGameProcess")
		.method("register_player!", &Ju_GameProcessWrapper::registerPlayer)
		.method("init!", &Ju_GameProcessWrapper::init)
		.method("reset!", &Ju_GameProcessWrapper::reset)
		.method("observe", &Ju_GameProcessWrapper::observe)
		.method("release!", &Ju_GameProcessWrapper::release);

	/* Enums */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_bits<ObserverType>("ObserverType");
	mod.set_const("NONE", ObserverType::NONE);
	mod.set_const("SPRITE_2D", ObserverType::SPRITE_2D);
	mod.set_const("BLOCK_2D", ObserverType::BLOCK_2D);
	mod.set_const("VECTOR", ObserverType::VECTOR);

	/* NumpyWrapper */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<NumpyWrapper<uint8_t>>("NumpyWrapper")
		.method("shape", &NumpyWrapper<uint8_t>::getShape)
		.method("strides", &NumpyWrapper<uint8_t>::getStrides);

}