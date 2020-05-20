import numpy as np

from griddle_python import RenderToFile, gd, griddle_loader

if __name__ == '__main__':

    gdy = griddle_loader()

    gdy_description = gdy.load('yaml/bg_tiling.yaml')

    grid = gdy_description.load_level(0)

    renderWindow = RenderToFile()

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    observation = np.array(game.reset(), copy=False)

    renderWindow.render(observation, "bg_test.png")



