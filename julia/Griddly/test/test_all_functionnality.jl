using Griddly
using Test
using BenchmarkTools
# include("../src/Griddly.jl")

image_path = joinpath(@__DIR__,"..","..","..","resources","images")
shader_path = joinpath(@__DIR__,"..","..","..","resources","shaders")
gdy_path = joinpath(@__DIR__,"..","..","..","resources","games")

@testset "Griddly" begin

	gdy_reader = Griddly.GDYReader(image_path,shader_path)
	grid = Griddly.load!(gdy_reader,joinpath(gdy_path,"Single-Player/Mini-Grid/minigrid-doggo.yaml"))
	# Set of Test to see if we load from file correctly and if basic functions works
	@testset "LoadFromFile" begin
    	@test Griddly.get_tile_size(grid) == 24
    	@test Griddly.get_all_available_actions(grid) == ["move"]
    	@test Griddly.get_player_available_actions(grid) == ["move"]
    	@test Griddly.get_non_player_available_actions(grid) == []
    	@test Griddly.get_input_ids(grid,"move") == [1,2,3]
    	@test Griddly.get_avatar_object(grid) =="doggo"
    	Griddly.set_tile_size!(grid,16)
    	@test Griddly.get_tile_size(grid) == 16
        Griddly.set_tile_size!(grid,24)
    end

    @testset "Game Entity" begin
        game = Griddly.create_game(grid,Griddly.SPRITE_2D)
        @test Griddly.get_player_count(grid) == 1
        @test Griddly.get_num_players(game) == 0
        player1 = Griddly.register_player!(game,"Bob", Griddly.BLOCK_2D)
        @test Griddly.get_num_players(game) == 1
        Griddly.init!(game)
        Griddly.load_level!(grid,1)
        Griddly.reset!(game)
        @test Griddly.get_width(grid) == 7
        @test Griddly.get_height(grid) == 7

		# test a random game with different level
		function run()
			for l in 0:4
				Griddly.load_level!(grid,l)
				Griddly.reset!(game)

				for j in 1:1000
					dir = rand(0:5)

					reward, done = Griddly.step_player!(player1,"move", [dir])
					# observation = Griddly.observe(game)
				end
			end
		end

        # test if the render tool is working
		function run_window()
	        render_window = RenderWindow(700,700)
	        for l in 0:4
	            Griddly.load_level!(grid,l)
	            Griddly.reset!(game)
	            observation = Griddly.observe(game)
	            observation = Griddly.get_data(observation)
	            for j in 1:1000
	    	        dir = rand(0:5)

	    	        reward, done = Griddly.step_player!(player1,"move", [dir])

	                observation = Griddly.observe(game)
	                observation = Griddly.get_data(observation)
	                render(render_window,observation)
	    	    end
	        end
		end

		function run_window_player()
	        render_window_player = RenderWindow(700,700)
	        for l in 1:5
	            Griddly.load_level!(grid,l)
	            Griddly.reset!(game)
	            observation = Griddly.observe(game)
	            observation = Griddly.get_data(observation)
	            for j in 1:1000
	                dir = rand(0:5)

	                reward, done = Griddly.step_player!(player1,"move", [dir])

	                player1_tiles = Griddly.observe(player1)
	                player1_tiles = Griddly.get_data(player1_tiles)
	                render(render_window_player, player1_tiles)
	            end
	        end
		end


        # test if we can capture video
		function run_video()
			video = VideoRecorder((700,700),"test_video";saving_path="Griddly/test/")
	        io = start(video)
	        Griddly.load_level!(grid,1)
	        Griddly.reset!(game)
	        observation = Griddly.observe(game)
	        observation = Griddly.get_data(observation)
	        for j in 1:1000
	            dir = rand(0:5)

	            reward, done = Griddly.step_player!(player1,"move", [dir])

	            player1_tiles = Griddly.observe(player1)
	            observation = Griddly.observe(game)
	            observation = Griddly.get_data(observation)
	            add_frame!(video,io,observation)
	        end
	        save(video,io)
			# # test if we can save a frame
			# Griddly.save_frame(video.scene,"test_capture_frame";file_path="Griddly/test/",format=".jpg")
		end

        # function load_allocation()
        #     Griddly.load_level!(grid,1)
        # end
        # function reset_allocation()
        #     Griddly.reset!(game)
        #     Griddly.reset!(game)
        #     Griddly.reset!(game)
        # end
		#
        # function step_allocation()
        #     for j in 1:5000
        #         dir = rand(0:5)
        #         reward, done = Griddly.step_player!(player1,"move", [dir])
		# 		observation = Griddly.observe(game)
	    #         observation = Griddly.get_data(observation)
        #     end
        # end
		#
        # function observe_allocation()
        #     # player1_tiles = Griddly.observe(player1)
        #     # player1_tiles = Griddly.get_data(player1_tiles)
        #     observation = Griddly.observe(game)
        #     observation = Griddly.get_data(observation)
        # end

        println(@timed run_window())

    end

#     gdy_string = """Version: "0.1"
# Environment:
#   Name: GriddlyRTS
#   TileSize: 16
#   BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_panel_metal_a.png
#   Player:
#     Count: 2
#   Termination:
#     Lose:
#       - eq: [base:count, 0] # If the player has no bases
#     Win:
#       - eq: [_score, 10] # First player to 10 reward points
#   Levels:
#     - |
#       W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
#       W  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  M  M  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  M  .  M  M  M  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  .  .  M  .  M  M  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  W
#       W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
#     - |
#       W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
#       W  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  M  M  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  M  .  M  M  M  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
#       W  W  W  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
#       W  .  .  .  .  .  .  W  W  .  .  .  w  w  w  w  w  w  .  .  .  .  .  W  W  W  W  W  w  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  w  w  .  .  .  .  .  .  .  W  W  W  W  W  w  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  W  W  W  W  W  w  W
#       W  .  .  .  .  P1 w  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  W  W  W  W  W  W  W  W
#       W  .  .  .  .  P1 w  .  .  .  .  .  .  .  M  .  .  M  .  M  M  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  P2 w  .  .  .  .  .  .  .  M  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  P2 w  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  P1 w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
#       W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  W
#       W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

# Actions:
#   - Name: gather
#     Behaviours:
#       - Src:
#           Object: harvester
#           Commands:
#             - incr: resources
#             - reward: 1
#         Dst:
#           Object: minerals
#           Commands:
#             - decr: resources
#       - Src:
#           Object: harvester
#           Commands:
#             - decr: resources
#             - reward: 1
#         Dst:
#           Object: base
#           Commands:
#             - incr: resources

#   - Name: move
#     Behaviours:
#       - Src:
#           Object: [harvester, puncher, pusher, movable_wall]
#           Commands:
#             - mov: _dest # mov will move the object, _dest is the destination location of the action
#         Dst:
#           Object: _empty

#       - Src:
#           Object: pusher
#           Commands:
#             - mov: _dest # mov will move the object, _dest is the destination location of the action
#         Dst:
#           Object: [movable_wall, harvester, puncher]
#           Commands:
#             - cascade: _dest # reapply the same action to the dest location of the action

#   - Name: punch
#     Behaviours:
#       - Src:
#           Object: puncher
#           Commands:
#             - reward: 1
#         Dst:
#           Object: [puncher, harvester, pusher, base]
#           Commands:
#             - decr: health
#             - eq:
#                 Arguments: [0, health]
#                 Commands:
#                   - remove: true

# Objects:
#   - Name: minerals
#     MapCharacter: M
#     Variables:
#       - Name: resources
#         InitialValue: 10
#     Observers:
#       Sprite2D:
#         - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png
#       Block2D:
#         - Shape: triangle
#           Color: [0.0, 1.0, 0.0]
#           Scale: 1.0

#   - Name: harvester
#     MapCharacter: H
#     Variables:
#       - Name: resources
#         InitialValue: 0
#       - Name: health
#         InitialValue: 10
#     Observers:
#       Sprite2D:
#         - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_jelly_d1.png
#       Block2D:
#         - Shape: square
#           Color: [0.6, 0.2, 0.2]
#           Scale: 0.5

#   - Name: pusher
#     MapCharacter: P
#     Variables:
#       - Name: health
#         InitialValue: 10
#     Observers:
#       Sprite2D:
#         - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_crawler_queen_d1.png
#       Block2D:
#         - Shape: square
#           Color: [0.2, 0.2, 0.6]
#           Scale: 1.0

#   - Name: puncher
#     MapCharacter: p
#     Variables:
#       - Name: health
#         InitialValue: 5
#     Observers:
#       Sprite2D:
#         - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png
#       Block2D:
#         - Color: [0.2, 0.6, 0.6]
#           Shape: square
#           Scale: 0.8

#   - Name: fixed_wall
#     MapCharacter: W
#     Observers:
#       Sprite2D:
#         - TilingMode: WALL_2 # Will tile walls with two images
#           Image:
#             - oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img33.png
#             - oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img40.png
#       Block2D:
#         - Color: [0.5, 0.5, 0.5]
#           Shape: square

#   - Name: movable_wall
#     MapCharacter: w
#     Observers:
#       Sprite2D:
#         - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img282.png
#       Block2D:
#         - Color: [0.8, 0.8, 0.8]
#           Shape: square

#   - Name: base
#     MapCharacter: B
#     Variables:
#       - Name: resources
#         InitialValue: 0
#       - Name: health
#         InitialValue: 10
#     Observers:
#       Sprite2D:
#         - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img324.png
#       Block2D:
#         - Color: [0.8, 0.8, 0.3]
#           Shape: triangle"""

#     grid2 = Griddly.load_string!(gdy_reader,gdy_string)
#     @test Griddly.get_tile_size(grid2) == 16
end
