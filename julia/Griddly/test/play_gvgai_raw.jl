using Griddly

image_path = joinpath(@__DIR__,"..","..","..","resources","images")
shader_path = joinpath(@__DIR__,"..","..","..","resources","shaders")
gdy_path = joinpath(@__DIR__,"..","..","..","resources","games")

gdy_reader = Griddly.GDYReader(image_path,shader_path)
grid = Griddly.load!(gdy_reader,joinpath(gdy_path,"Single-Player/Mini-Grid/minigrid-doggo.yaml"))

println("Grid is loaded, go to create the game now")

game = Griddly.create_game(grid,Griddly.SPRITES_2D)

println("Game is created, we now register a player")

# Create a player
player1 = Griddly.register_player!(game,"Bob", Griddly.BLOCK_2D)

println("Player is created, now we initialize the  game")

Griddly.init!(game)

println("Game is initialized now we want to play on the 5 gvgai levels each for 1000 timesteps")

frames = 0
start = time_ns()

render_window = RenderWindow(700,700)

for l in 0:4
    Griddly.load_level!(grid,l)
    Griddly.reset!(game)
    observation = Griddly.observe(game)
    println(convert(Array{Int8,3},Griddly.get_data(observation)))
    for j in 1:200
        dir = rand(0:5)

        reward, done = Griddly.step_player!(player1,"move", [dir])

        # player1_tiles = Griddly.observe(player1)

        observation = Griddly.observe(game)
        println(convert(Array{Int8,3},Griddly.get_data(observation)))
        render(render_window,observation)

        frames += 1

        if (frames % 100 == 0)
            over = time_ns()
            println("fps: $(frames / (over - start))")
            frames = 0
            start = time_ns()
        end
    end
