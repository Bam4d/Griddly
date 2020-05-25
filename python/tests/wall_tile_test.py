import sys
import os
import numpy as np

from griddly_python import gd, griddly_loader
from griddly_python.RenderTools import RenderToFile

if __name__ == '__main__':

    loader = griddly_loader()

    game_description = loader.load_game_description('tests/walls.yaml')

    grid = game_description.load_level(0)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    renderToFile = RenderToFile()

    game.init()
    observation = np.array(game.reset(), copy=False)

    renderToFile.render(observation, "python/wall_test.png")


