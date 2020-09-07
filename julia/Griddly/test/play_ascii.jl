using Griddly

image_path = joinpath(@__DIR__,"..","..","..","resources","images")
shader_path = joinpath(@__DIR__,"..","..","..","resources","shaders")
gdy_path = joinpath(@__DIR__,"..","..","..","resources","games")

gdy_reader = Griddly.GDYReader(image_path,shader_path)
# grid = Griddly.load!(gdy_reader,joinpath(gdy_path,"Single-Player/Mini-Grid/minigrid-doggo.yaml"))
#
# println("Grid is loaded, go to create the game now")
#
# game = Griddly.create_game(grid,Griddly.ASCII)
#
# println("Game is created, we now register a player")
#
# # Create a player
# player1 = Griddly.register_player!(game,"Bob", Griddly.BLOCK_2D)
#
# println("Player is created, now we initialize the  game")
#
# Griddly.init!(game)
#
# println("Game is initialized now we want to play and observe with an ASCII representation")
#
# Griddly.load_level!(grid,2)
# Griddly.reset!(game)
# list_object_ids = Griddly.get_object_ids_list(grid)
# println("list_ids:$(convert(Array{Int32,1},list_object_ids))")
# object1_char = Griddly.get_object_char_from_id(grid,1)
# println("object1_char:$object1_char")
# mapping = Griddly.get_map_object_ids_char(grid)
# println(mapping)
# observation = Griddly.observe(game)
# observation = convert(Array{Int8,3},Griddly.get_data(observation))
# Griddly.pretty_print_ASCII(grid,mapping,observation)
# for j in 1:200
#     dir = rand(0:5)
#
#     reward, done = Griddly.step_player!(player1,"move", [dir])
#
#     observation = Griddly.observe(game)
#     Griddly.pretty_print_ASCII(grid,mapping,convert(Array{Int8,3},Griddly.get_data(observation)))
# end

#--------------Player ASCII--------------
grid = Griddly.load!(gdy_reader,joinpath(gdy_path,"Single-Player/Mini-Grid/minigrid-doggo.yaml"))

println("Grid is loaded, go to create the game now")

game = Griddly.create_game(grid,Griddly.SPRITE_2D)

println("Game is created, we now register a player")

# Create a player
player1 = Griddly.register_player!(game,"Bob", Griddly.ASCII)

println("Player is created, now we initialize the  game")

Griddly.init!(game)

println("Game is initialized now we want to play and observe with an ASCII representation")

Griddly.load_level!(grid,2)
Griddly.reset!(game)
list_object_ids = Griddly.get_object_ids_list(grid)
println("list_ids:$(convert(Array{Int32,1},list_object_ids))")
object1_char = Griddly.get_object_char_from_id(grid,1)
println("object1_char:$object1_char")
mapping = Griddly.get_map_object_ids_char(grid)
println(mapping)

for j in 1:200
    dir = rand(0:5)

    reward, done = Griddly.step_player!(player1,"move", [dir])

    player1_tiles = Griddly.observe(player1)
    Griddly.pretty_print_ASCII(grid,mapping,convert(Array{Int8,3},Griddly.get_data(player1_tiles)))
end

#--------------2PlayersGame------------------")
#
# grid = Griddly.load!(gdy_reader,joinpath(gdy_path,"RTS/basicRTS.yaml"))
#
# println("Grid is loaded, go to create the game now")
#
# game = Griddly.create_game(grid,Griddly.ASCII)
#
# println("Game is created, we now register a player")
#
# # Create a player
# player1 = Griddly.register_player!(game,"Bob", Griddly.BLOCK_2D)
# player2 = Griddly.register_player!(game,"Alice", Griddly.BLOCK_2D)
#
# println("Players are created, now we initialize the  game")
#
# Griddly.init!(game)
#
# # Load a custom string
# level_string = """  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  M  M  M
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  M  M
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  M
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  B1 .  .  .  .  .  .  .  M  W  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  W  W  W  W  W  .  .  .  .
#                     .  .  .  .  W  W  W  W  W  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  B2 .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  M  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     M  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     M  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
#                     M  M  M  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
# """
# Griddly.load_level_string!(grid,level_string)
# Griddly.reset!(game)
#
# println("The game is ready to be played")
#
# list_object_ids = Griddly.get_object_ids_list(grid)
# println("list_ids:$(convert(Array{Int32,1},list_object_ids))")
# object1_char = Griddly.get_object_char_from_id(grid,1)
# println("object1_char:$object1_char")
# mapping = Griddly.get_map_object_ids_char(grid)
# println(mapping)
# actions_map = Griddly.get_player_actions_dict(grid)
# println(actions_map)
#
# function run_test()
#     player1_x::Int64 = 6
#     player1_y::Int64 = 11
#
#     player2_x::Int64 = 25
#     player2_y::Int64 = 19
#
#     for j in 1:200
#
#         # Alternate between player_1 and player_2 actions
#         action = collect(keys(actions_map))[rand(1:length(actions_map))]
#         dir = rand(1:length(actions_map[action]))
#         println("action:$action,dir:$dir,player_id:$(j%2+1)")
#         if j % 2 == 0
#             action_dir = [player1_x,player1_y,dir]
#             player_1_step_result = Griddly.step_player!(player1,action,action_dir)
#             if action=="move"
#                 if dir==1
#                     player1_x -=1
#                 elseif dir==2
#                     player1_y -=1
#                 elseif dir ==3
#                     player1_x +=1
#                 else
#                     player1_y +=1
#                 end
#             end
#         else
#             action_dir = [player2_x,player2_y,dir]
#             player_2_step_result = Griddly.step_player!(player2,action,action_dir)
#             if action=="move"
#                 if dir==1
#                     player2_x -=1
#                 elseif dir==2
#                     player2_y -=1
#                 elseif dir ==3
#                     player2_x +=1
#                 else
#                     player2_y +=1
#                 end
#             end
#         end
#
#         observation = Griddly.observe(game)
#         Griddly.pretty_print_ASCII(grid,mapping,convert(Array{Int8,3},Griddly.get_data(observation)))
#     end
# end
# run_test()
