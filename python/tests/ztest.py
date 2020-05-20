import sys
import os
import numpy as np

# The griddy lib is in the build directory when built so add it and then import
from griddle_python import RenderToFile, gd, griddle_loader


if __name__ == '__main__':

    gdy = griddle_loader()

    gdy_description = gdy.load('yaml/ztest.yaml')

    # grid = gdy_description.load_level(0)
    grid = gdy_description.create_level(5, 5)

    renderWindow = RenderToFile()

    for x in range(0, 5):
        for y in range(0, 5):
            grid.add_object(1, x, y, "ghost")

    for x in range(1, 4):
        for y in range(1, 4):
            grid.add_object(1, x, y, "floor")

    game = grid.create_game(gd.ObserverType.BLOCK_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    observation = np.array(game.reset(), copy=False)
    renderWindow.render(observation, "ztest.png")



