import sys
import os
import numpy as np

from griddle_python import RenderToFile, gd, griddle_loader

if __name__ == '__main__':

    gdy = griddle_loader()

    gdy_description = gdy.load('yaml/walls.yaml')

    grid = gdy_description.load_level(0)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    renderToFile = RenderToFile()

    game.init()
    observation = np.array(game.reset(), copy=False)

    renderToFile.render(observation, "python/wall_test.png")

