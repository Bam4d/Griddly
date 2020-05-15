import sys
import os
import numpy as np
import pyglet
import pyglet.gl as gl
from timeit import default_timer as timer

# The griddy lib is in the build directory when built so add it and then import
from tools import RenderWindow

sys.path.extend([os.path.join(os.getcwd(), 'Debug/bin')])


import python_griddle as gd

window = None


if __name__ == '__main__':

    # print(gd.version)

    # gym.make('griddy-sokoban-lvl0-v0')

    width = 30
    height = 30

    renderWindow = RenderWindow(32 * width, 32 * height)

    gdy = gd.GDYReader()

    gdy_description = gdy.load('resources/games/RTS/basicRTS.yaml')

    grid = gdy_description.create_level(width, height)

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(-1, x, y, "minerals")

    for i in range(0, 100):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(0, x, y, "movable_wall")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "harvester")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "puncher")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "pusher")

    for i in range(0, 100):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(0, x, y, "fixed_wall")

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)
    player2 = game.add_player('Alice', gd.ObserverType.NONE)

    game.init()

    game.reset()

    # observation = np.array(game.observe(), copy=False)

    # renderWindow.render(observation)

    start = timer()

    frames = 0

    # Player objects have the same interface as gym environments
    for i in range(0, 100000):
        for j in range(0, 10000):
            x = np.random.randint(width)
            y = np.random.randint(height)
            dir = np.random.randint(4)

            reward = player1.step("move", [x,y,dir])

            #player1_tiles = player1.observe()

            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            frames += 1

            if frames % 1000 == 0:
                end = timer()
                print(f'fps: {frames / (end-start)}')
                frames = 0
                start = timer()


        game.reset()

