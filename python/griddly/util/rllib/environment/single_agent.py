from typing import Any, Dict, List, Optional, Tuple, Union

import numpy as np
import numpy.typing as npt

from griddly.gym import GymWrapper
from griddly.spaces.action_space import MultiAgentActionSpace
from griddly.spaces.observation_space import MultiAgentObservationSpace
from griddly.typing import Action, ActionSpace, Observation, ObservationSpace
from griddly.util.rllib.environment.base import _RLlibEnv
from griddly.util.rllib.environment.observer_episode_recorder import \
    ObserverEpisodeRecorder


class RLlibEnv(_RLlibEnv):
    """
    Wraps a Griddly environment for compatibility with RLLib.

    Use the `env_config` in the rllib config to provide Griddly Environment Parameters

    Example:

    Firstly register the RLlibWrapper using rllib's

    env_name = "my_env_name"

    register_env(env_name, RLlibWrapper)

    you can then configure it

    rllib_config = {
        'env_config': {
            'yaml_file': 'Single-Player/GVGAI/butterflies.yaml',
            'level": 6,
            'player_observer_type': gd.ObserverType.SPRITE_2D,
            'global_observer_type': gd.ObserverType.ISOMETRIC,
            'max_steps': 1000,
        },
        # Other configuration options
    }

    Create the rllib trainer using this config:

    trainer = ImpalaTrainer(rllib_config, env=env_name)

    """

    def __init__(self, env_config: Dict[str, Any]) -> None:
        super().__init__(env_config)

        self.reset()

    def _transform(
        self, observation: Union[List[Observation], Observation]
    ) -> Union[List[Observation], Observation]:
        transformed_obs: Union[List[Observation], Observation]
        if self._env.player_count > 1 and isinstance(observation, list):
            transformed_obs = []
            for obs in observation:
                assert isinstance(
                    obs, npt.NDArray
                ), "When using RLLib, observations must be numpy arrays, such as VECTOR or SPRITE_2D"
                transformed_obs.append(obs.transpose(1, 2, 0).astype(float))
        elif isinstance(observation, npt.NDArray):
            transformed_obs = observation.transpose(1, 2, 0).astype(float)
        else:
            raise Exception(
                f"Unsupported observation type {type(observation)} for {self.__class__.__name__}"
            )

        return transformed_obs

    def _after_step(
        self,
        observation: Union[List[Observation], Observation],
        reward: Union[List[int], int],
        done: bool,
        info: Dict[str, Any],
    ) -> Dict[str, Any]:
        extra_info: Dict[str, Any] = {}

        if self.is_video_enabled():
            videos_list = []
            if self.include_agent_videos:
                video_info = self._agent_recorder.step(
                    self._env.level_id, self.env_steps, done
                )
                if video_info is not None:
                    videos_list.append(video_info)
            if self.include_global_video and self._global_recorder is not None:
                video_info = self._global_recorder.step(
                    self._env.level_id, self.env_steps, done
                )
                if video_info is not None:
                    videos_list.append(video_info)

            self.videos = videos_list

        return extra_info

    def reset(
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ) -> Tuple[Union[List[Observation], Observation], Dict[Any, Any]]:
        if options is None:
            options = {}
        if self._level_generator is not None:
            options["level_string"] = self._level_generator.generate()
        elif self._random_level_on_reset:
            options["level_id"] = np.random.choice(self._env.level_count)

        self._rllib_cache.reset()
        observation, info = self._env.reset(seed=seed, options=options)

        if self.generate_valid_action_trees:
            self.last_valid_action_trees = self._get_valid_action_trees()

        return self._transform(observation), info

    def step(
        self, action: Action
    ) -> Tuple[
        Union[List[Observation], Observation],
        Union[List[int], int],
        bool,
        bool,
        Dict[Any, Any],
    ]:
        observation, reward, truncated, done, info = self._env.step(action)

        extra_info = self._after_step(observation, reward, done, info)
        info.update(extra_info)

        if self.generate_valid_action_trees:
            self.last_valid_action_trees = self._get_valid_action_trees()
            info["valid_action_tree"] = self.last_valid_action_trees.copy()

        self.env_steps += 1

        return self._transform(observation), reward, done, truncated, info

    def render(self) -> Union[str, npt.NDArray]:
        return self._env.render()

    def is_video_enabled(self) -> bool:
        return (
            self.record_video_config is not None
            and self._env_idx is not None
            and self._env_idx == 0
        )

    def on_episode_start(self, worker_idx: int, env_idx: int) -> None:
        self._env_idx = env_idx
        self._worker_idx = worker_idx

        if self.is_video_enabled() and not self.video_initialized:
            self.init_video_recording()
            self.video_initialized = True

    def init_video_recording(self) -> None:
        
        if self.include_agent_videos:
            self._agent_recorder = ObserverEpisodeRecorder(
                self._env, 1, self.video_frequency, self.video_directory, self.fps
            )
        if self.include_global_video:
            self._global_recorder = ObserverEpisodeRecorder(
                self._env,
                "global",
                self.video_frequency,
                self.video_directory,
                self.fps,
            )