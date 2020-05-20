import numpy as np
from datetime import datetime

from griddle_python import RenderWindow, griddle_loader, gd

window = None

if __name__ == '__main__':

    loader = griddle_loader()

    game_description = loader.load_game_description('RTS/basicRTS.yaml')

    grid = game_description.load_level(0)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    game.reset()

    width = grid.get_width()
    height = grid.get_height()

    renderWindow = RenderWindow(70 * width, 70 * height)

    observation = np.array(game.observe(), copy=False)

    renderWindow.render(observation)

    start = datetime.now()

    frames = 0

    # Player objects have the same interface as gym environments
    for i in range(0, 100000):
        for j in range(0, 10000):
            x = np.random.randint(width)
            y = np.random.randint(height)
            dir = np.random.randint(5)

            reward, done = player1.step("move", [dir])

            player1_tiles = player1.observe()

            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            if reward != 0:
                print(f'reward: {reward} done: {done}')

            frames += 1

            if frames % 1000 == 0:
                end = datetime.now()
                if (end - start).seconds > 0:
                    print(f'fps: {frames / (end - start).seconds}')

        game.reset()


