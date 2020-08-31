module Griddly

    using CxxWrap
    # @wrapmodule(joinpath(@__DIR__,"..","..","..","Debug","bin","JuGriddly"),:define_module_jugriddly)
	@wrapmodule(joinpath(@__DIR__,"..","..","..","Release","bin","JuGriddly"),:define_module_jugriddly)

    function __init__()
   		@initcxx
    end

	function get_player_actions_dict(grid)
		dict = Dict{String,Array{Int32,1}}()
		player_actions = convert(Array{String,1},Griddly.get_player_available_actions(grid))
		for action_name in player_actions
			action_input = convert(Array{Int32,1},Griddly.get_input_ids(grid,action_name))
			dict[action_name] = action_input
		end
		return dict
	end
	
    ######################################## RenderTool ########################################
    using Makie
	using FixedPointNumbers
	using ImageCore

	include("rendertools.jl")
	export RenderWindow, VideoRecorder, init, render, save_frame, start, add_frame!, save_video, MultipleScreen, render_multiple

end
