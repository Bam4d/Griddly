#using Griddly

image_path = joinpath(@__DIR__,"..","..","resources","images") 
shader_path = joinpath(@__DIR__,"..","..","resources","shaders")
gdy_path = joinpath(@__DIR__,"..","..","resources","games")

gdy_reader = Griddly.GDYReader(image_path,shader_path) 
grid = Griddly.load!(gdy_reader,joinpath(gdy_path,"Single-Player/Mini-Grid/minigrid-doggo.yaml"))

println("Grid is loaded, go to create the game now")

game = Griddly.create_game(grid,Griddly.SPRITE_2D)

println("Game is created, we now register a player")

# Create a player
player1 = Griddly.register_player!(game,"Bob", Griddly.BLOCK_2D)

println("Player is created, now we initialize the  game")

Griddly.init!(game)

println("Game is initialized, now we want to load the 5 gvgai levels")

for l in 1:5
    Griddly.load_level!(grid,l-1)
    println("level is loaded")
    Griddly.reset!(game)
    println("game is reset")
    observation = Griddly.observe(game)
    for j in 1:1000
        dir = rand(0:5)

        reward, done = Griddly.step!(player1,"move", [dir])

        player1_tiles = Griddly.observe(player1)
    end
end