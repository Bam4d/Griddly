import cv2
from gym.wrappers.monitoring.video_recorder import ImageEncoder


class RenderWindow():

    def __init__(self, width, height):
        super().__init__()
        self._width = width
        self._height = height

        self._pyglet = __import__('pyglet')
        self._gl = self._pyglet.gl

        self._initialized = False

    def init(self):
        if not self._initialized:
            self._initialized = True
            self._window = self._pyglet.window.Window(width=self._width,
                                                      height=self._height,
                                                      vsync=False,
                                                      resizable=False)

    def render(self, observation):

        if not self._initialized:
            self.init()

        obs = observation.swapaxes(0, 2)
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
        self._gl.glTexParameteri(self._gl.GL_TEXTURE_2D, self._gl.GL_TEXTURE_MAG_FILTER, self._gl.GL_NEAREST)
        self._gl.glTexParameteri(self._gl.GL_TEXTURE_2D, self._gl.GL_TEXTURE_MIN_FILTER, self._gl.GL_NEAREST)
        self._window.switch_to()

        self._window.dispatch_events()
        texture.blit(0, 0)  # draw
        self._window.flip()


class RenderToFile():

    def __init__(self):
        super().__init__()

    def render(self, observation, string_filename):
        image = cv2.cvtColor(observation.swapaxes(0, 2), cv2.COLOR_RGB2BGR)
        cv2.imwrite(string_filename, image)


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
        height = observation_shape[2]
        width = observation_shape[1]
        pixFmt = observation_shape[0]

        frame_shape = (height, width, pixFmt)
        self._image_encoder = ImageEncoder(output_file, frame_shape, fps, fps)

    def add_frame(self, observation):
        """
        :param observation:
        :return:
        """
        self._image_encoder.capture_frame(observation.swapaxes(0, 2))

    def close(self):
        self._image_encoder.close()

    def __del__(self):
        # Release everything if job is finished
        self.close()
