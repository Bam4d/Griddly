import os
import sys

import numpy as np

# The griddy lib is in the build directory when built so add it and then import
from tools import RenderToFile

sys.path.extend([os.path.join(os.getcwd(), 'Debug/bin')])

import griddy as gd


if __name__ == '__main__':

    gdy = gd.GDYReader()

    gdy_description = gdy.load('tests/resources/bg_tiling.yaml')

    grid = gdy_description.load_level(0)

    renderWindow = RenderToFile()

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    observation = np.array(game.reset(), copy=False)

    renderWindow.render(observation, "python/bg_test.png")



