import numpy as np

from griddly import GriddlyLoader, gd
from griddly.RenderTools import RenderToFile

if __name__ == '__main__':
    # A nice tool to save png images
    file_renderer = RenderToFile()

    # The entry point for all raw interfaces
    loader = GriddlyLoader()

    # Load the game description
    game_description = loader.load_game_description('RTS/basicRTS.yaml')

    # Create a new level
    width = 10
    height = 10
    grid = game_description.create_level(width, height)

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(-1, x, y, "minerals")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(0, x, y, "movable_wall")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "harvester")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "puncher")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "pusher")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(0, x, y, "fixed_wall")

    # This defines the global observer
    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Defines the player and their observers
    player = game.add_player('Bob', gd.ObserverType.BLOCK_2D)

    game.init()
    game.reset()

    global_observation = np.array(game.observe(), copy=False)
    player_observation = np.array(player.observe(), copy=False)

    file_renderer.render(global_observation, "pcg_global_observer.png")
    file_renderer.render(player_observation, "pcg_player_observer.png")
