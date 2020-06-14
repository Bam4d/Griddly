import sys
import os
import numpy as np

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderToFile

if __name__ == '__main__':

    loader = GriddlyLoader()

    game_description = loader.load_game_description('tests/walls.yaml')

    grid = game_description.load_level(0)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    renderToFile = RenderToFile()

    game.init()
    observation = np.array(game.reset(), copy=False)

    renderToFile.render(observation, "python/wall_test.png")


