import imageio
from gym.wrappers.monitoring.video_recorder import ImageEncoder
import os


class RenderWindow():

    def __init__(self, width, height, caption="Griddly"):
        super().__init__()
        self._width = width
        self._height = height
        self._caption = caption

        self._pyglet = __import__('pyglet')
        self._gl = self._pyglet.gl

        self._initialized = False

    def init(self):
        if not self._initialized:
            self._initialized = True

            self._window = self._pyglet.window.Window(
                caption=self._caption,
                width=self._width,
                height=self._height,
                resizable=False
            )

    def render(self, observation):

        if not self._initialized:
            self.init()

        obs = observation.swapaxes(0, 2)

        self._window.switch_to()
        image = self._pyglet.image.ImageData(obs.shape[1],
                                             obs.shape[0],
                                             'RGB',
                                             obs.tobytes(),
                                             pitch=obs.shape[1] * -3
                                             )

        texture = image.get_texture()
        texture.width = self._width
        texture.height = self._height
        self._window.clear()
        self._gl.glTexParameteri(
            self._gl.GL_TEXTURE_2D, self._gl.GL_TEXTURE_MAG_FILTER, self._gl.GL_NEAREST)
        self._gl.glTexParameteri(
            self._gl.GL_TEXTURE_2D, self._gl.GL_TEXTURE_MIN_FILTER, self._gl.GL_NEAREST)

        self._window.dispatch_events()
        texture.blit(0, 0)  # draw
        self._window.flip()

    def close(self):
        if self._initialized:
            self._window.close()

    def __del__(self):
        self.close()


class RenderToFile():

    def __init__(self):
        super().__init__()

    def render(self, observation, string_filename):
        imageio.imwrite(string_filename, observation)


class VideoRecorder():
    """
    Use ImageEncoder gym.wrappers.monitoring.video_recorder because it make really nice videos using .mp4 and ffmpeg
    """

    def start(self, output_file, observation_shape, fps=30):
        """
        :param output_file:
        :param observation_shape:
        :param fps:
        :return:
        """
        self.output_file = output_file
        self._image_encoder = ImageEncoder(output_file, observation_shape, fps, fps)

    def add_frame(self, observation):
        """
        :param observation:
        :return:
        """
        self._image_encoder.capture_frame(observation)

    def close(self):
        self._image_encoder.close()

    def __del__(self):
        # Release everything if job is finished
        self.close()
