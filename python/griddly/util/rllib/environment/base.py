import os
from abc import ABC
from typing import Any, Dict, List, Optional, Union

from griddly.gym import GymWrapper
from griddly.spaces.action_space import MultiAgentActionSpace
from griddly.spaces.observation_space import MultiAgentObservationSpace
from griddly.typing import ActionSpace, ObservationSpace
from griddly.util.rllib.environment.observer_episode_recorder import (
    ObserverEpisodeRecorder,
)


class _RLlibEnvCache:
    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self.action_space: Optional[Union[ActionSpace, MultiAgentActionSpace]] = None
        self.observation_space: Optional[
            Union[ObservationSpace, MultiAgentObservationSpace]
        ] = None


class _RLlibEnv(ABC):
    def __init__(self, env_config: Dict[str, Any]) -> None:
        self._rllib_cache = _RLlibEnvCache()

        self._env = GymWrapper(**env_config, reset=False)

        self.env_config = env_config

        self.env_steps = 0
        self._agent_recorders: Optional[
            Union[ObserverEpisodeRecorder, List[ObserverEpisodeRecorder]]
        ] = None
        self._global_recorder: Optional[ObserverEpisodeRecorder] = None

        self._env_idx: Optional[int] = None
        self._worker_idx: Optional[int] = None

        self.video_initialized = False

        self.record_video_config = env_config.get("record_video_config", None)

        self.videos: List[Dict[str, Any]] = []

        if self.record_video_config is not None:
            self.video_frequency = self.record_video_config.get("frequency", 1000)
            self.fps = self.record_video_config.get("fps", 10)
            self.video_directory = os.path.realpath(
                self.record_video_config.get("directory", ".")
            )
            self.include_global_video = self.record_video_config.get(
                "include_global", True
            )
            self.include_agent_videos = self.record_video_config.get(
                "include_agents", False
            )
            os.makedirs(self.video_directory, exist_ok=True)

        self.record_actions = env_config.get("record_actions", False)

        self.generate_valid_action_trees = env_config.get(
            "generate_valid_action_trees", False
        )
        self._random_level_on_reset = env_config.get("random_level_on_reset", False)
        level_generator_rllib_config = env_config.get("level_generator", None)

        self._level_generator = None
        if level_generator_rllib_config is not None:
            level_generator_class = level_generator_rllib_config["class"]
            level_generator_config = level_generator_rllib_config["config"]
            self._level_generator = level_generator_class(level_generator_config)

        self._env.enable_history(self.record_actions)

    @property
    def width(self) -> int:
        assert self._env.observation_space.shape is not None
        return self._env.observation_space.shape[0]

    @property
    def height(self) -> int:
        assert self._env.observation_space.shape is not None
        return self._env.observation_space.shape[1]

    def _get_valid_action_trees(self) -> Union[Dict[str, Any], List[Dict[str, Any]]]:
        valid_action_trees = self._env.game.build_valid_action_trees()
        if self._env.player_count == 1:
            return valid_action_trees[0]
        return valid_action_trees
