import os
from enum import Enum
from typing import Any, Dict, Union, Optional
from uuid import uuid1

from griddly.gym import GymWrapper
from griddly.util.render_tools import RenderToVideo
from griddly.wrappers import RenderWrapper


class RecordingState(Enum):
    NOT_RECORDING = 1
    WAITING_FOR_EPISODE_START = 2
    BEFORE_RECORDING = 3
    RECORDING = 4


class ObserverEpisodeRecorder:
    def __init__(
        self,
        env: GymWrapper,
        observer: Union[str, int],
        video_frequency: int,
        video_directory: str = ".",
        fps: int = 10,
    ) -> None:
        self._video_frequency = video_frequency
        self._video_directory = video_directory
        self._observer = observer
        self._env = RenderWrapper(env, observer, "rgb_array")
        self._fps = fps

        self._recording_state = RecordingState.BEFORE_RECORDING
        self._recorder: RenderToVideo

    def step(self, level_id: str, step_count: int, done: bool) -> Optional[Dict[str, Any]]:
        video_info = None

        if (
            self._recording_state is RecordingState.NOT_RECORDING
            and step_count % self._video_frequency == 0
        ):
            self._recording_state = RecordingState.WAITING_FOR_EPISODE_START

        if self._recording_state == RecordingState.BEFORE_RECORDING:
            video_filename = os.path.join(
                self._video_directory,
                f"episode_video_{self._observer}_{uuid1()}_{level_id}_{step_count}.mp4",
            )

            self._recorder = RenderToVideo(self._env, video_filename)

            self._recording_state = RecordingState.RECORDING

        if self._recording_state == RecordingState.RECORDING:
            self._recorder.capture_frame()
            if done:
                self._recording_state = RecordingState.NOT_RECORDING
                self._recorder.close()

                video_info = {"level": level_id, "path": self._recorder.path}

        if self._recording_state == RecordingState.WAITING_FOR_EPISODE_START:
            if done:
                self._recording_state = RecordingState.BEFORE_RECORDING

        return video_info

    def __del__(self) -> None:
        self._recorder.close()
