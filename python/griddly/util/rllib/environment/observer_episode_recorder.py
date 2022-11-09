from enum import Enum
import os
from uuid import uuid1

from griddly.RenderTools import VideoRecorder


class RecordingState(Enum):
    NOT_RECORDING = 1
    WAITING_FOR_EPISODE_START = 2
    BEFORE_RECORDING = 3
    RECORDING = 4


class ObserverEpisodeRecorder:
    def __init__(self, env, observer, video_frequency, video_directory="."):

        self._video_frequency = video_frequency
        self._video_directory = video_directory
        self._observer = observer
        self._env = env

        self._recording_state = RecordingState.BEFORE_RECORDING

    def step(self, level_id, step_count, done):

        video_info = None

        if (
            self._recording_state is RecordingState.NOT_RECORDING
            and step_count % self._video_frequency == 0
        ):
            self._recording_state = RecordingState.WAITING_FOR_EPISODE_START

        if self._recording_state == RecordingState.BEFORE_RECORDING:
            obs = self._env.render(observer=self._observer, mode="rgb_array")
            self._recorder = VideoRecorder()

            video_filename = os.path.join(
                self._video_directory,
                f"episode_video_{self._observer}_{uuid1()}_{level_id}_{step_count}.mp4",
            )

            self._recorder.start(video_filename, obs.shape)
            self._recording_state = RecordingState.RECORDING

        if self._recording_state == RecordingState.RECORDING:
            obs = self._env.render(observer=self._observer, mode="rgb_array")
            self._recorder.add_frame(obs)
            if done:
                self._recording_state = RecordingState.NOT_RECORDING
                self._recorder.close()

                video_info = {"level": level_id, "path": self._recorder.output_file}

        if self._recording_state == RecordingState.WAITING_FOR_EPISODE_START:
            if done:
                self._recording_state = RecordingState.BEFORE_RECORDING

        return video_info
