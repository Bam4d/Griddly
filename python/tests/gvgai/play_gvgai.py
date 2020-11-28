import numpy as np
from timeit import default_timer as timer

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderWindow

window = None

if __name__ == '__main__':

    loader = GriddlyLoader()

    gdy = loader.load('Single-Player/GVGAI/spider-nest.yaml')

    game = gdy.create_game(gd.ObserverType.BLOCK_2D)

    #grid.set_max_steps(100)

    # Create a player
    player1 = game.register_player('Bob', gd.ObserverType.BLOCK_2D)

    game.load_level(0)
    game.init()

    renderWindow = RenderWindow(1200, 500)

    start = timer()

    frames = 0

    # Player objects have the same interface as gym environments
    for l in range(0, 5000):
        game.load_level(l)
        game.reset()
        observation = np.array(game.observe(), copy=False)
        #renderWindow.render(observation)
        for j in range(0, 100000):
            dir = np.random.randint(5)

            reward, done, info = player1.step("move", [dir])

            #player1_tiles = np.array(player1.observe(), copy=False)

            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            # if reward != 0:
            #     print(f'reward: {reward} done: {done}')

            frames += 1

            if frames % 100 == 0:
                end = timer()
                print(f'fps: {frames / (end - start)}')
                frames = 0
                start = timer()





