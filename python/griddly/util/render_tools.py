from typing import Optional

import imageio
from moviepy.video.io.ImageSequenceClip import ImageSequenceClip

from griddly import GymWrapper

class RenderToVideo:
    def __init__(
        self,
        env: GymWrapper,
        path: Optional[str] = None,
    ):
        self.render_history = []
        self.env = env
        self.path = path

        self.frames_per_sec = env.metadata.get("render_fps", 30)
        self.recorded_frames = []

    def capture_frame(self):
        """Render the given `env` and add the resulting frame to the video."""
        frame = self.env.render()

        self.recorded_frames.append(frame)

    def close(self):
        # Close the encoder
        if len(self.recorded_frames) > 0:
            clip = ImageSequenceClip(self.recorded_frames, fps=self.frames_per_sec)
            clip.write_videofile(self.path)

        self.recorded_frames = []


class RenderToWindow:
    def __init__(self, width, height, caption="Griddly"):
        super().__init__()
        self._width = width
        self._height = height
        self._caption = caption

        self._pyglet = __import__("pyglet")
        self._gl = self._pyglet.gl

        self._initialized = False

    def init(self):
        if not self._initialized:
            self._initialized = True

            self._window = self._pyglet.window.Window(
                caption=self._caption,
                width=self._width,
                height=self._height,
                resizable=False,
            )

    def render(self, observation):
        if not self._initialized:
            self.init()

        obs = observation.swapaxes(0, 2)

        self._window.switch_to()
        image = self._pyglet.image.ImageData(
            obs.shape[1], obs.shape[0], "RGB", obs.tobytes(), pitch=obs.shape[1] * -3
        )

        texture = image.get_texture()
        texture.width = self._width
        texture.height = self._height
        self._window.clear()
        self._gl.glTexParameteri(
            self._gl.GL_TEXTURE_2D, self._gl.GL_TEXTURE_MAG_FILTER, self._gl.GL_NEAREST
        )
        self._gl.glTexParameteri(
            self._gl.GL_TEXTURE_2D, self._gl.GL_TEXTURE_MIN_FILTER, self._gl.GL_NEAREST
        )

        self._window.dispatch_events()
        texture.blit(0, 0)  # draw
        self._window.flip()

    def close(self):
        if self._initialized:
            self._window.close()

    def __del__(self):
        self.close()


class RenderToFile:
    def __init__(self):
        super().__init__()

    def render(self, observation, string_filename):
        imageio.imwrite(string_filename, observation)
