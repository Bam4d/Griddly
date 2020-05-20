import numpy as np

# The griddy lib is in the build directory when built so add it and then import
from griddle_python import RenderWindow, gd, griddle_loader


if __name__ == '__main__':

    # print(gd.version)
    level_string ="""W  W  W  W  W 
W  W  W  .  W 
W  P1 w  .  W 
W  .  .  W  W 
W  W  W  W  W 
"""

    width = 5
    height = 5

    renderWindow = RenderWindow(32 * width, 32 * height)

    gdy = griddle_loader()

    gdy_description = gdy.load('../griddle_python/resources/games/RTS/basicRTS.yaml')

    grid = gdy_description.load_level_string(level_string)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()
    observation = np.array(game.reset(), copy=False)

    renderWindow.render(observation)

    reward1 = player1.step(1, 2, "move", gd.Direction.RIGHT)

    observation = np.array(game.observe(), copy=False)
    renderWindow.render(observation)

    reward2 = player1.step(2, 2, "move", gd.Direction.RIGHT)

    observation = np.array(game.observe(), copy=False)
    renderWindow.render(observation)

    reward3 = player1.step(3, 2, "move", gd.Direction.RIGHT)

    observation = np.array(game.observe(), copy=False)
    renderWindow.render(observation)

