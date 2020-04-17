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


    width = 30
    height = 30

    renderWindow = RenderWindow(32*width, 32*height)

    grid = gd.Grid(width, height)

    grid.add_object(1, 5, 5, gd.ObjectType.PUSHER)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    game.reset()

    #observation = np.array(game.observe(), copy=False)

    #renderWindow.render(observation)

    # Player objects have the same interface as gym environments

    for i in range(5,10):
        observation = np.array(game.observe(), copy=False)
        renderWindow.render(observation)
        reward = player1.step(i, 5, gd.ActionType.MOVE, gd.Direction.RIGHT)



