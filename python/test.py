import sys
import os
import numpy as np
import pyglet
import pyglet.gl as gl

# The griddy lib is in the build directory when built so add it and then import
sys.path.extend([os.path.join(os.getcwd(), 'build')])

import griddy as gd

window = None


class RenderWindow():

    def __init__(self, width, height):
        self._width = width
        self._height = height

        self._window = pyglet.window.Window(width=self._width,
                                            height=self._height,
                                            vsync=False,
                                            resizable=True)

    def render(self, observation):
        obs = observation.swapaxes(0,2)
        image = pyglet.image.ImageData(obs.shape[0],
                                       obs.shape[1],
                                       'RGB',
                                       obs.tobytes(),
                                       pitch=obs.shape[0] * -3
                                       )
        texture = image.get_texture()
        texture.width = self._width
        texture.height = self._height
        self._window.clear()
        gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MAG_FILTER, gl.GL_NEAREST)
        gl.glTexParameteri(gl.GL_TEXTURE_2D, gl.GL_TEXTURE_MIN_FILTER, gl.GL_NEAREST)
        self._window.switch_to()
        self._window.dispatch_events()
        texture.blit(0, 0)  # draw
        self._window.flip()


if __name__ == '__main__':

    # print(gd.version)

    # gym.make('griddy-sokoban-lvl0-v0')

    width = 30
    height = 30

    renderWindow = RenderWindow(32*width, 32*height)

    map_reader = gd.MapReader()

    grid = map_reader.load_map_file('resources/levels/harvester2.txt')

    # Not gym Interface here...
    # grid = gd.Grid(width, height)
    #
    # for i in range(0, 1):
    #     x = np.random.randint(width)
    #     y = np.random.randint(height)
    #
    #     grid.add_object(-1, x, y, gd.ObjectType.FIXED_WALL)
    #
    # for i in range(0, 100):
    #     x = np.random.randint(width)
    #     y = np.random.randint(height)
    #
    #     grid.add_object(0, x, y, gd.ObjectType.PUSHER)
    #
    # for i in range(0, 1):
    #     x = np.random.randint(width)
    #     y = np.random.randint(height)
    #
    #     grid.add_object(1, x, y, gd.ObjectType.HARVESTER)
    #
    # for i in range(0, 100):
    #     x = np.random.randint(width)
    #     y = np.random.randint(height)
    #
    #     grid.add_object(-1, x, y, gd.ObjectType.PUSHABLE_WALL)
    #
    # for i in range(0, 1):
    #     x = np.random.randint(width)
    #     y = np.random.randint(height)
    #
    #     grid.add_object(-1, x, y, gd.ObjectType.MINERALS)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)
    player2 = game.add_player('Alice', gd.ObserverType.VECTOR)

    game.start_game()

    #observation = np.array(game.observe(), copy=False)

    #renderWindow.render(observation)

    # Player objects have the same interface as gym environments
    for i in range(0, 100):
        x = np.random.randint(width)
        y = np.random.randint(height)
        reward = player1.step(x, y, gd.ActionType.MOVE, gd.Direction.RIGHT)

        observation = np.array(game.observe(), copy=False)
        renderWindow.render(observation)

    #player1.render()

    # player2.step()
    # player2.render()
    #
    # game.render()

    # grid.add_object(player1, gd., 0, 0)

    #
    # # Add an object at a particular location, the object might be owned by a player
    # grid.add_object(player, 'OBJECT_TYPE', 0, 0)
    #
    # # Create an action belonging to a plyer
    # #action = player.create_action(...)
    #
    # # Create an observer
    # observer = gd.create_observer('thing')
    #
    # # Create a game from the previous settings
    # game = gd.create_game_process([player], observer, grid)
    #
    # game.perform_actions([action])
