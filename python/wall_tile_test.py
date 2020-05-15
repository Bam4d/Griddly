import sys
import os
import numpy as np

# The griddy lib is in the build directory when built so add it and then import
from tools import RenderToFile

sys.path.extend([os.path.join(os.getcwd(), 'Debug/bin')])

import griddle as gd


if __name__ == '__main__':

    gdy = gd.GDYReader()

    gdy_description = gdy.load('tests/resources/walls.yaml')

    grid = gdy_description.load_level(0)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    renderToFile = RenderToFile()

    game.init()
    observation = np.array(game.reset(), copy=False)

    renderToFile.render(observation, "python/wall_test.png")


