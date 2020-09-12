#include <jlcxx/jlcxx.hpp>
#include <jlcxx/const_array.hpp>
#include <jlcxx/stl.hpp>


#include <spdlog/spdlog.h>
#include <sstream>
#include <memory>
#include <vector>
// Griddly module
#include "wrapper/NumpyWrapper.cpp"
#include "wrapper/JuGDYReaderWrapper.cpp"
#include "wrapper/JuGridWrapper.cpp"
#include "wrapper/JuStepPlayerWrapper.cpp"

using namespace griddly;

namespace jlcxx {
	template<> struct IsMirroredType<ObserverType> : std::true_type {};
	template<typename ScalarT>
	struct BuildParameterList<NumpyWrapper<ScalarT>>
	{
		typedef ParameterList<ScalarT> type;
	};
} // namespace jlcxx


namespace griddly {

JLCXX_MODULE define_module_jugriddly(jlcxx::Module& mod) {

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::debug);
#else
	spdlog::set_level(spdlog::level::info);
#endif
	
	spdlog::debug("Julia Griddly module loaded!");
	using namespace jlcxx;

	/* Enums */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_bits<ObserverType>("ObserverType", jlcxx::julia_type("CppEnum"));
	mod.set_const("NONE", ObserverType::NONE);
	mod.set_const("SPRITE_2D", ObserverType::SPRITE_2D);
	mod.set_const("BLOCK_2D", ObserverType::BLOCK_2D);
	mod.set_const("VECTOR", ObserverType::VECTOR);
	mod.set_const("ASCII", ObserverType::ASCII);

	/* NumpyWrapper */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<Parametric<TypeVar<1>>>("NumpyWrapper")
		.apply<NumpyWrapper<uint8_t>>([&mod](auto wrapped)
		{
			typedef typename decltype(wrapped)::type WrappedT;
			typedef typename WrappedT::NumpyWrapper ScalarT;

			mod.method("get_shape", [](ScalarT& nw) {return nw.getShape(); });
			mod.method("get_strides", [](ScalarT& nw) {return nw.getStrides(); });
			mod.method("get_scalar_size", [](ScalarT& nw) {return nw.getScalarSize(); });
			mod.method("get_data", [](ScalarT& nw) {
			 	std::vector<uint32_t> shape = nw.getShape();
			 	int_t nDims = shape[0];
			 	int_t nCols = shape[1];
			 	int_t nRows = shape[2];
			 	return jlcxx::make_julia_array(nw.getData(), nDims, nCols, nRows);
			});
		});
	//mod.add_type<NumpyWrapper<uint8_t>>("NumpyWrapper")
	//	.method("get_shape", [](NumpyWrapper<uint8_t>& nw) {return nw.getShape(); })
	//	.method("get_strides", [](NumpyWrapper<uint8_t>& nw) {return nw.getStrides(); })
	//	.method("get_scalar_size", [](NumpyWrapper<uint8_t>& nw) {return nw.getScalarSize(); })
	//	.method("get_data", [](NumpyWrapper<uint8_t>& nw) {
	//		std::vector<uint32_t> shape = nw.getShape();
	//		int_t nDims = shape[0];
	//		int_t nCols = shape[1];
	//		int_t nRows = shape[2];
	//		return jlcxx::make_const_array(nw.getData(), nDims, nCols, nRows);
	//		});

	/* StepPlayer */
	/*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<Ju_StepPlayerWrapper>("Player")
	//	.method("observe", &Ju_StepPlayerWrapper::observe)
	//	.method("step_player!", [](Ju_StepPlayerWrapper& jlstepplayer, std::string action_name, jlcxx::ArrayRef<int_t, 1> actions_array)
	//	  	{
	//	  		 std::vector<int32_t> data(actions_array.begin(), actions_array.end());
	//			 std::tuple<int, bool> step = jlstepplayer.step(action_name, data);
	//	  		return step;
	//	  	});

	mod.add_type<Ju_StepPlayerWrapper>("Player")
		.method("observe", [](Ju_StepPlayerWrapper& jlstepplayer) {return jlstepplayer.observe(); })
		.method("step_player!", [](Ju_StepPlayerWrapper& jlstepplayer, std::string action_name, jlcxx::ArrayRef<int_t, 1> actions_array)
			{
				std::vector<int32_t> data(actions_array.begin(), actions_array.end());
				std::tuple<int, bool> step = jlstepplayer.step(action_name, data);
				return step;
			});

	/* GameProcess */
	/*----------------------------------------------------------------------------------------------------------------*/
	//mod.add_type<Ju_GameProcessWrapper>("GameProcess")
	//	.method("register_player!", &Ju_GameProcessWrapper::registerPlayer)
	//	.method("get_num_players", &Ju_GameProcessWrapper::getNumPlayers)
	//	.method("init!", &Ju_GameProcessWrapper::init)
	//	.method("reset!", &Ju_GameProcessWrapper::reset)
	//	.method("observe", &Ju_GameProcessWrapper::observe)
	//	.method("release!", &Ju_GameProcessWrapper::release);

	mod.add_type<Ju_GameProcessWrapper>("GameProcess")
		.method("register_player!", [](Ju_GameProcessWrapper& jlgpw, std::string playerName, ObserverType observerType) 
			{
				return jlgpw.registerPlayer(playerName, observerType); 
			})
		.method("get_num_players", [](Ju_GameProcessWrapper& jlgpw) {return jlgpw.getNumPlayers(); })
		.method("init!", [](Ju_GameProcessWrapper& jlgpw) {return jlgpw.init(); })
		.method("reset!", [](Ju_GameProcessWrapper& jlgpw) {return jlgpw.reset(); })
		.method("observe", [](Ju_GameProcessWrapper& jlgpw) {return jlgpw.observe(); })
		.method("release!", [](Ju_GameProcessWrapper& jlgpw) {return jlgpw.release(); });

	/* Grid */
	/*----------------------------------------------------------------------------------------------------------------*/
	/*mod.add_type<Ju_GridWrapper>("Grid")
		.method("set_tile_size!", &Ju_GridWrapper::setTileSize)
		.method("get_tile_size", &Ju_GridWrapper::getTileSize)
		.method("get_width", &Ju_GridWrapper::getWidth)
		.method("get_height", &Ju_GridWrapper::getHeight)
		.method("get_player_count", &Ju_GridWrapper::getPlayerCount)
		.method("get_all_available_actions", &Ju_GridWrapper::getAllAvailableAction)
		.method("get_player_available_actions", &Ju_GridWrapper::getPlayerAvailableAction)
		.method("get_non_player_available_actions", &Ju_GridWrapper::getNonPlayerAvailableAction)
		.method("get_input_ids", &Ju_GridWrapper::getInputsIds)
		.method("get_avatar_object", &Ju_GridWrapper::getAvatarObject)
		.method("create_level", &Ju_GridWrapper::createLevel)
		.method("load_level!", &Ju_GridWrapper::loadLevel)
		.method("load_level_string!", &Ju_GridWrapper::loadLevelString)
		.method("create_game", &Ju_GridWrapper::createGame)
		.method("add_object!", &Ju_GridWrapper::addObject)
		;*/

	mod.add_type<Ju_GridWrapper>("Grid")
		.method("set_tile_size!", [](Ju_GridWrapper& jlgw, uint32_t tileSize) {return jlgw.setTileSize(tileSize); })
		.method("get_tile_size", [](Ju_GridWrapper& jlgw) {return jlgw.getTileSize(); })
		.method("get_width", [](Ju_GridWrapper& jlgw) {return jlgw.getWidth(); })
		.method("get_height", [](Ju_GridWrapper& jlgw) {return jlgw.getHeight(); })
		.method("get_player_count", [](Ju_GridWrapper& jlgw) {return jlgw.getPlayerCount(); })
		.method("get_all_available_actions", [](Ju_GridWrapper& jlgw) {return jlgw.getAllAvailableAction(); })
		.method("get_player_available_actions", [](Ju_GridWrapper& jlgw) {return jlgw.getPlayerAvailableAction(); })
		.method("get_non_player_available_actions", [](Ju_GridWrapper& jlgw) {return jlgw.getNonPlayerAvailableAction(); })
		.method("get_input_ids", [](Ju_GridWrapper& jlgw, std::string actionName) {return jlgw.getInputsIds(actionName); })
		.method("get_avatar_object", [](Ju_GridWrapper& jlgw) {return jlgw.getAvatarObject(); })
		.method("create_level", [](Ju_GridWrapper& jlgw, uint32_t width, uint32_t height) {return jlgw.createLevel(width,height); })
		.method("load_level!", [](Ju_GridWrapper& jlgw, uint32_t level) {return jlgw.loadLevel(level); })
		.method("load_level_string!", [](Ju_GridWrapper& jlgw, std::string levelString) {return jlgw.loadLevelString(levelString); })
		.method("create_game", [](Ju_GridWrapper& jlgw, ObserverType observerType) {return jlgw.createGame(observerType); })
		.method("add_object!", [](Ju_GridWrapper& jlgw, int playerId, int32_t startX, int32_t startY, std::string objectName) 
			{
				return jlgw.addObject(playerId, startX, startY, objectName); 
			})
		.method("get_object_ids_list", [](Ju_GridWrapper& jlgw) {return jlgw.getObjectIdsList(); })
		.method("get_object_char_from_id", [](Ju_GridWrapper& jlgw, uint32_t objectId) {return jlgw.getObjectCharFromId(objectId);})
		;

	/* GDYFactory */
	/*----------------------------------------------------------------------------------------------------------------*/
	mod.add_type<Ju_GDYReaderWrapper>("GDYReader")
		.constructor<std::string, std::string>()
		.method("load!", &Ju_GDYReaderWrapper::loadGDYFile)
		.method("load_string!", &Ju_GDYReaderWrapper::loadGDYString);
	//mod.add_type<Ju_GDYReaderWrapper>("GDYReader")
	//	.constructor<std::string, std::string>()
	//	.method("load!", [](Ju_GDYReaderWrapper& jlgdy, std::string filename) {return jlgdy.loadGDYFile(filename); })
	//	.method("load_string!", [](Ju_GDYReaderWrapper& jlgdy, std::string string) {return jlgdy.loadGDYString(string); });

}
}
