import sys
import os
import numpy as np

# The griddy lib is in the build directory when built so add it and then import
from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderToFile

if __name__ == '__main__':

    loader = GriddlyLoader()

    game_description = loader.load_game_description('resources/ztest.yaml')

    # grid = gdy_description.load_level(0)
    grid = game_description.create_level(5, 5)

    renderWindow = RenderToFile()

    for x in range(0, 5):
        for y in range(0, 5):
            grid.add_object(1, x, y, "ghost")

    for x in range(1, 4):
        for y in range(1, 4):
            grid.add_object(1, x, y, "floor")

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.register_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    observation = np.array(game.reset(), copy=False)
    renderWindow.render(observation, "ztest.png")



