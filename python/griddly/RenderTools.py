import pyglet
import pyglet.gl as gl
import cv2
import numpy as np

class PyGletImageTool():

    def __init__(self):
        pass

    def _get_image(self, observation):
        """
        :param observation: Image in CWH format
        :return:
        """
        obs = observation.swapaxes(0, 2)
        image = pyglet.image.ImageData(obs.shape[1],
                                       obs.shape[0],
                                       'RGB',
                                       obs.tobytes(),
                                       pitch=obs.shape[1] * -3
                                       )
        return image

class RenderWindow(PyGletImageTool):

    def __init__(self, width, height):
        super().__init__()
        self._width = width
        self._height = height

        self._initialized = False

    def init(self):
        if not self._initialized:
            self._initialized = True
            self._window = pyglet.window.Window(width=self._width,
                                                height=self._height,
                                                vsync=False,
                                                resizable=False)

    def render(self, observation):

        if not self._initialized:
            self.init()

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

class VideoRecorder():
    """
    Use open CV to record frames to make videos
    """
    def __init__(self):

        # Define the codec and create VideoWriter object
        self._fourcc = cv2.VideoWriter_fourcc(*"mp4v")

        self._video_out = None


    def start(self, output_file, observation_shape, fps=30):
        """
        :param output_file:
        :param observation_shape:
        :param fps:
        :return:
        """
        height = observation_shape[2]
        width = observation_shape[1]
        self._video_out = cv2.VideoWriter(output_file, self._fourcc, fps, (width, height))
        self._video_out.set(cv2.VIDEOWRITER_PROP_QUALITY,100)


    def add_frame(self, observation):
        """
        :param observation:
        :return:
        """
        if self._video_out is None:
            raise RuntimeWarning("Recording must be started with start() method before adding frames")

        # Write the frame
        converted_image = cv2.cvtColor(observation.swapaxes(0,2), cv2.COLOR_RGB2BGR)
        self._video_out.write(converted_image)

    def __del__(self):
        # Release everything if job is finished
        if self._video_out is not None:
            self._video_out.release()