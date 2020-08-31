using Griddly
using Test
using Makie
# include("../src/Griddly.jl")
# using BenchmarkTools

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
				end
			end
		end
		println(@timed run())

		@testset "RenderTools" begin

	        Griddly.load_level!(grid,1)
	        Griddly.reset!(game)
	        # test if the render tool is working
			function run_window()
		        for l in 0:4
					render_window = RenderWindow(700,700)
		            Griddly.load_level!(grid,l)
		            Griddly.reset!(game)
		            observation = Griddly.observe(game)
		            observation = Griddly.get_data(observation)
		            for j in 1:800
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
		        for l in 0:4
		            Griddly.load_level!(grid,l)
		            Griddly.reset!(game)
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
				video = VideoRecorder((700,700),"test_video2";saving_path="julia/Griddly/test/")
		        io = start(video)
		        Griddly.load_level!(grid,4)
		        Griddly.reset!(game)
		        observation = Griddly.observe(game)
		        observation = Griddly.get_data(observation)
		        for j in 1:1000
		            dir = rand(0:5)

		            reward, done = Griddly.step_player!(player1,"move", [dir])

		            observation = Griddly.observe(game)
		            observation = Griddly.get_data(observation)
		            add_frame!(video,io,observation)
		        end
		        save_video(video,io)
			end

			# test if we can render several observation
			function run_multiple_screen()
				Griddly.load_level!(grid,3)
				Griddly.reset!(game)
				screen = MultipleScreen(1000,1000;nb_scene=5)
				for j in 1:200
					dir = rand(0:5)

					reward, done = Griddly.step_player!(player1,"move", [dir])

					observation = Griddly.observe(game)
					observation = Griddly.get_data(observation)
					player1_tiles = Griddly.observe(player1)
					player1_tiles = Griddly.get_data(player1_tiles)
					render_multiple(screen, [player1_tiles,player1_tiles,player1_tiles,observation,observation])
				end
			end

			# test if we can save a frame
			function run_save_frame()
				Griddly.load_level!(grid,3)
				Griddly.reset!(game)
				observation = Griddly.observe(game)
				observation = Griddly.get_data(observation)
				for j in 1:10
					dir = rand(0:5)

					reward, done = Griddly.step_player!(player1,"move", [dir])

					observation = Griddly.observe(game)
					observation = Griddly.get_data(observation)
				end
				Griddly.save_frame(observation,(700,700),"test_capture_frame2";file_path="julia/Griddly/test/",format=".jpg")
			end

	        # println(@timed run_multiple_screen())
		end
    end
	@testset "LoadFromString & Multiplayer" begin
	    gdy_string = """Version: "0.1"
	Environment:
	  Name: GriddlyRTS
	  TileSize: 16
	  BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_panel_metal_a.png
	  Player:
	    Count: 2
	  Termination:
	    Lose:
	      - eq: [base:count, 0] # If the player has no bases
	    Win:
	      - eq: [_score, 10] # First player to 10 reward points
	  Levels:
	    - |
	      W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
	      W  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  M  M  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  M  .  M  M  M  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  .  .  M  .  M  M  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  W
	      W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
	    - |
	      W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
	      W  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  M  M  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  M  .  M  M  M  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
	      W  W  W  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
	      W  .  .  .  .  .  .  W  W  .  .  .  w  w  w  w  w  w  .  .  .  .  .  W  W  W  W  W  w  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  w  w  .  .  .  .  .  .  .  W  W  W  W  W  w  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  W  W  W  W  W  w  W
	      W  .  .  .  .  P1 w  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  W  W  W  W  W  W  W  W
	      W  .  .  .  .  P1 w  .  .  .  .  .  .  .  M  .  .  M  .  M  M  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  P2 w  .  .  .  .  .  .  .  M  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  P2 w  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  P1 w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
	      W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  W
	      W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

	Actions:
	  - Name: gather
	    Behaviours:
	      - Src:
	          Object: harvester
	          Commands:
	            - incr: resources
	            - reward: 1
	        Dst:
	          Object: minerals
	          Commands:
	            - decr: resources
	      - Src:
	          Object: harvester
	          Commands:
	            - decr: resources
	            - reward: 1
	        Dst:
	          Object: base
	          Commands:
	            - incr: resources

	  - Name: move
	    Behaviours:
	      - Src:
	          Object: [harvester, puncher, pusher, movable_wall]
	          Commands:
	            - mov: _dest # mov will move the object, _dest is the destination location of the action
	        Dst:
	          Object: _empty

	      - Src:
	          Object: pusher
	          Commands:
	            - mov: _dest # mov will move the object, _dest is the destination location of the action
	        Dst:
	          Object: [movable_wall, harvester, puncher]
	          Commands:
	            - cascade: _dest # reapply the same action to the dest location of the action

	  - Name: punch
	    Behaviours:
	      - Src:
	          Object: puncher
	          Commands:
	            - reward: 1
	        Dst:
	          Object: [puncher, harvester, pusher, base]
	          Commands:
	            - decr: health
	            - eq:
	                Arguments: [0, health]
	                Commands:
	                  - remove: true

	Objects:
	  - Name: minerals
	    MapCharacter: M
	    Variables:
	      - Name: resources
	        InitialValue: 10
	    Observers:
	      Sprite2D:
	        - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png
	      Block2D:
	        - Shape: triangle
	          Color: [0.0, 1.0, 0.0]
	          Scale: 1.0

	  - Name: harvester
	    MapCharacter: H
	    Variables:
	      - Name: resources
	        InitialValue: 0
	      - Name: health
	        InitialValue: 10
	    Observers:
	      Sprite2D:
	        - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_jelly_d1.png
	      Block2D:
	        - Shape: square
	          Color: [0.6, 0.2, 0.2]
	          Scale: 0.5

	  - Name: pusher
	    MapCharacter: P
	    Variables:
	      - Name: health
	        InitialValue: 10
	    Observers:
	      Sprite2D:
	        - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_crawler_queen_d1.png
	      Block2D:
	        - Shape: square
	          Color: [0.2, 0.2, 0.6]
	          Scale: 1.0

	  - Name: puncher
	    MapCharacter: p
	    Variables:
	      - Name: health
	        InitialValue: 5
	    Observers:
	      Sprite2D:
	        - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png
	      Block2D:
	        - Color: [0.2, 0.6, 0.6]
	          Shape: square
	          Scale: 0.8

	  - Name: fixed_wall
	    MapCharacter: W
	    Observers:
	      Sprite2D:
	        - TilingMode: WALL_2 # Will tile walls with two images
	          Image:
	            - oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img33.png
	            - oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img40.png
	      Block2D:
	        - Color: [0.5, 0.5, 0.5]
	          Shape: square

	  - Name: movable_wall
	    MapCharacter: w
	    Observers:
	      Sprite2D:
	        - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img282.png
	      Block2D:
	        - Color: [0.8, 0.8, 0.8]
	          Shape: square

	  - Name: base
	    MapCharacter: B
	    Variables:
	      - Name: resources
	        InitialValue: 0
	      - Name: health
	        InitialValue: 10
	    Observers:
	      Sprite2D:
	        - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img324.png
	      Block2D:
	        - Color: [0.8, 0.8, 0.3]
	          Shape: triangle"""

	    grid = Griddly.load_string!(gdy_reader,gdy_string)
	    @test Griddly.get_tile_size(grid) == 16
		@test Griddly.get_all_available_actions(grid) == ["gather","move","punch"]
    	@test Griddly.get_player_available_actions(grid) == ["gather","move","punch"]
    	@test Griddly.get_non_player_available_actions(grid) == []
    	@test Griddly.get_input_ids(grid,"move") == [1,2,3,4]
    	@test Griddly.get_input_ids(grid,"punch") == [1,2,3,4]
		@test Griddly.get_input_ids(grid,"gather") == [1,2,3,4]
		actions_map = Griddly.get_player_actions_dict(grid)
		@test actions_map == Dict("move"=>[1,2,3,4],
								  "punch"=>[1,2,3,4],
								  "gather"=>[1,2,3,4])
		game = Griddly.create_game(grid,Griddly.SPRITE_2D)
        @test Griddly.get_player_count(grid) == 2
        @test Griddly.get_num_players(game) == 0
        player1 = Griddly.register_player!(game,"Bob", Griddly.BLOCK_2D)
        @test Griddly.get_num_players(game) == 1
		player2 = Griddly.register_player!(game,"Alice",Griddly.BLOCK_2D)
        @test Griddly.get_num_players(game) == 2
        Griddly.init!(game)
        Griddly.load_level!(grid,1)
        Griddly.reset!(game)
        @test Griddly.get_width(grid) == 30
        @test Griddly.get_height(grid) == 30

		# test a random game with different level and random action
		function run()
			for l in 0:1
				Griddly.load_level!(grid,l)
				Griddly.reset!(game)
				for j in 1:1000
					x = rand(1:Griddly.get_width(grid))
					y = rand(1:Griddly.get_height(grid))

					# Alternate between player_1 and player_2 actions
					action = collect(keys(actions_map))[rand(1:length(actions_map))]
					dir = rand(1:length(actions_map[action]))
					println("action:$action,dir:$dir")
			        if j % 2 == 0
			            player_1_step_result = Griddly.step_player!(player1,action, [x, y, dir])
			        else
			            player_2_step_result = Griddly.step_player!(player2,action, [x, y, dir])
					end
				end
			end
		end
		# println(@timed run())

		@testset "RenderTool" begin
			function run_multiple_screen()
				Griddly.load_level!(grid,1)
				Griddly.reset!(game)
				screen = MultipleScreen(1000,1000;nb_scene=3)
				for j in 1:1000
					x = rand(1:Griddly.get_width(grid))
					y = rand(1:Griddly.get_height(grid))

					# Alternate between player_1 and player_2 actions
					action = collect(keys(actions_map))[rand(1:length(actions_map))]
					dir = rand(1:length(actions_map[action]))
					println("action:$action,dir:$dir")
			        if j % 2 == 0
			            player_1_step_result = Griddly.step_player!(player1,action, [x, y, dir])
			        else
			            player_2_step_result = Griddly.step_player!(player2,action, [x, y, dir])
					end
					player1_tiles = Griddly.get_data(Griddly.observe(player1))
					player2_tiles = Griddly.get_data(Griddly.observe(player2))
					observation = Griddly.get_data(Griddly.observe(game))
					render_multiple(screen,[player1_tiles, player2_tiles,observation])
				end
			end
			# println(@timed run_multiple_screen())
		end
	end
end
