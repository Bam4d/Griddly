import numpy as np

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderToFile

if __name__ == '__main__':

    loader = GriddlyLoader()

    game_description = loader.load_game_description('resources/bg_tiling.yaml')

    grid = game_description.load_level(0)

    renderWindow = RenderToFile()

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.register_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    observation = np.array(game.reset(), copy=False)

    renderWindow.render(observation, "bg_test.png")



