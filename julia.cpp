#include "jlcxx/jlcxx.hpp"

#include <spdlog/spdlog.h>
#include <sstream>
#include <memory>

// Griddly module
#include "wrapper/NumpyWrapper.cpp"
//#include "wrapper/wrapper.hpp"
//#include "wrapper/GDYReaderWrapper.cpp"
//#include "wrapper/GridWrapper.cpp"

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

	///* GDYFactory */
	///*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<GDYReaderWrapper>("GDYReader")
	//	.method("load", &GDYReaderWrapper::loadGDYFile)
	//	.method("load_string", &GDYReaderWrapper::loadGDYString);

	///* Grid */
	///*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<GridWrapper>("JuGrid")
	//	.method("set_tile_size!", &GridWrapper::setTileSize)
	//	.method("get_tile_size", &GridWrapper::getTileSize)
	//	.method("get_width", &GridWrapper::getWidth)
	//	.method("get_height", &GridWrapper::getHeight)
	//	.method("get_player_count", &GridWrapper::getPlayerCount)
	//	/*.method("get_action_input_mappings", &Ju_GridWrapper::getActionInputMappings)*/
	//	.method("get_avatar_object", &GridWrapper::getAvatarObject)
	//	.method("create_level", &GridWrapper::createLevel)
	//	.method("load_level!", &GridWrapper::loadLevel)
	//	.method("load_level_string!", &GridWrapper::loadLevelString)
	//	.method("create_game", &GridWrapper::createGame)
	//	.method("add_object!", &GridWrapper::addObject)
	//	;

	///* StepPlayer */
	///*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<StepPlayerWrapper>("StepPlayerWrapper")
	//	.method("observe", &StepPlayerWrapper::observe)
	//	.method("step!", [](StepPlayerWrapper& jlstepplayer, std::string action_name, jlcxx::ArrayRef<int_t, 1> actions_array)
	//		{
	//			std::vector<int32_t> data(actions_array.begin(), actions_array.end());
	//			return jlstepplayer.step(action_name, data);
	//		});

	///* GameProcess */
	///*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<GameProcessWrapper>("JuGameProcess")
	//	.method("register_player!", &GameProcessWrapper::registerPlayer)
	//	.method("init!", &GameProcessWrapper::init)
	//	.method("reset!", &GameProcessWrapper::reset)
	//	.method("observe", &GameProcessWrapper::observe)
	//	.method("release!", &GameProcessWrapper::release);

	///* Enums */
	///*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_bits<ObserverType>("ObserverType");
	//mod.set_const("NONE", ObserverType::NONE);
	//mod.set_const("SPRITE_2D", ObserverType::SPRITE_2D);
	//mod.set_const("BLOCK_2D", ObserverType::BLOCK_2D);
	//mod.set_const("VECTOR", ObserverType::VECTOR);

	///* NumpyWrapper */
	///*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<NumpyWrapper<uint8_t>>("NumpyWrapper")
	//	.method("shape", &NumpyWrapper<uint8_t>::getShape)
	//	.method("strides", &NumpyWrapper<uint8_t>::getStrides);

}