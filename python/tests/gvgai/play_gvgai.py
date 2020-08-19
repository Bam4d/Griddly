import numpy as np
from timeit import default_timer as timer

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderWindow, RenderToFile

window = None

if __name__ == '__main__':

    loader = GriddlyLoader()

    grid = loader.load_game('Single-Player/Mini-Grid/minigrid-doggo.yaml')

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.register_player('Bob', gd.ObserverType.BLOCK_2D)

    game.init()

    renderWindow = RenderWindow(700, 700)

    start = timer()

    frames = 0
    renderToFile = RenderToFile()
    # Player objects have the same interface as gym environments
    for l in range(0, 1):
        grid.load_level(l)
        game.reset()
        observation = np.array(game.observe(), copy=False)
        renderWindow.render(observation)
        for j in range(0, 100):
            dir = np.random.randint(5)

            reward, done = player1.step("move", [dir])

            player1_tiles = np.array(player1.observe(), copy=False)

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
    renderToFile.render(observation,"test.jpg")





