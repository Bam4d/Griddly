import numpy as np

from griddly_python import RenderToFile, gd, griddly_loader

if __name__ == '__main__':

    loader = griddly_loader()

    game_description = loader.load_game_description('tests/bg_tiling.yaml')

    grid = game_description.load_level(0)

    renderWindow = RenderToFile()

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    observation = np.array(game.reset(), copy=False)

    renderWindow.render(observation, "bg_test.png")



