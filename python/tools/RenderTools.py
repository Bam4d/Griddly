import pyglet
import pyglet.gl as gl


class PyGletImageTool():

    def __init__(self):
        pass

    def _get_image(self, observation):
        """
        :param observation: Image in CWH format
        :return:
        """
        obs = observation.swapaxes(0, 2)
        image = pyglet.image.ImageData(obs.shape[0],
                                       obs.shape[1],
                                       'RGB',
                                       obs.tobytes(),
                                       pitch=obs.shape[0] * -3
                                       )
        return image

class RenderWindow(PyGletImageTool):

    def __init__(self, width, height):
        super().__init__()
        self._width = width
        self._height = height

        self._window = pyglet.window.Window(width=self._width,
                                            height=self._height,
                                            vsync=False,
                                            resizable=True)

    def render(self, observation):

        image = self._get_image(observation)

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


class RenderToFile(PyGletImageTool):

    def __init__(self):
        super().__init__()

    def render(self, observation, string_filename):
        image = self._get_image(observation)
        image.save(string_filename)