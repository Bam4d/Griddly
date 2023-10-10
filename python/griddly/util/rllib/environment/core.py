import os
from collections import defaultdict
from typing import Any, Dict, List, Optional, Tuple, Union, Set

import numpy as np
import numpy.typing as npt
from ray.rllib import MultiAgentEnv
from ray.rllib.utils.typing import MultiAgentDict

from griddly.gym import GymWrapper
from griddly.spaces.action_space import MultiAgentActionSpace
from griddly.spaces.observation_space import MultiAgentObservationSpace
from griddly.typing import Action, ActionSpace, Observation, ObservationSpace
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


class RLlibEnv(GymWrapper):
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
        self._rllib_cache = _RLlibEnvCache()
        super().__init__(**env_config, reset=False)

        self.env_config = env_config

        self.env_steps = 0
        self._agent_recorders: Optional[ObserverEpisodeRecorder] = None
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

        self.reset()

        self.enable_history(self.record_actions)

    def _transform(
        self, observation: Union[List[Observation], Observation]
    ) -> Union[List[Observation], Observation]:
        transformed_obs: Union[List[Observation], Observation]
        if self.player_count > 1 and isinstance(observation, list):
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
                    self.level_id, self.env_steps, done
                )
                if video_info is not None:
                    videos_list.append(video_info)
            if self.include_global_video and self._global_recorder is not None:
                video_info = self._global_recorder.step(
                    self.level_id, self.env_steps, done
                )
                if video_info is not None:
                    videos_list.append(video_info)

            self.videos = videos_list

        return extra_info

    # @property
    # def action_space(self) -> Union[ActionSpace, MultiAgentActionSpace]:  # type: ignore
    #     if self._rllib_cache.action_space is None:
    #         if isinstance(super().action_space, list):
    #             self._rllib_cache.action_space = super().action_space[0]
    #         else:
    #             self._rllib_cache.action_space = super().action_space

    #     return self._rllib_cache.action_space

    # @property
    # def observation_space(self) -> Union[ObservationSpace, MultiAgentObservationSpace]:
    #     if self._rllib_cache.observation_space is None:
    #         obs_space = (
    #             super().observation_space[0]
    #             if self.player_count > 1
    #             else super().observation_space
    #         )
    #         self._rllib_cache.observation_space = gym.spaces.Box(
    #             obs_space.low.transpose((1, 2, 0)).astype(float),
    #             obs_space.high.transpose((1, 2, 0)).astype(float),
    #             dtype=float,
    #         )
    #     return self._rllib_cache.observation_space

    # @observation_space.setter
    # def observation_space(
    #     self, value: Union[ObservationSpace, MultiAgentObservationSpace]
    # ) -> None:
    #     self._rllib_cache.observation_space = value

    # @action_space.setter
    # def action_space(self, value: Union[ActionSpace, MultiAgentActionSpace]) -> None:
    #     self._rllib_cache.action_space = value

    @property
    def width(self) -> int:
        assert self.observation_space.shape is not None
        return self.observation_space.shape[0]

    @property
    def height(self) -> int:
        assert self.observation_space.shape is not None
        return self.observation_space.shape[1]

    def _get_valid_action_trees(self) -> Union[Dict[str, Any], List[Dict[str, Any]]]:
        valid_action_trees = self.game.build_valid_action_trees()
        if self.player_count == 1:
            return valid_action_trees[0]
        return valid_action_trees

    def reset(  # type: ignore
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ) -> Tuple[Union[List[Observation], Observation], Dict[Any, Any]]:
        if options is None:
            options = {}
        if self._level_generator is not None:
            options["level_string"] = self._level_generator.generate()
        elif self._random_level_on_reset:
            options["level_id"] = np.random.choice(self.level_count)

        self._rllib_cache.reset()
        observation, info = super().reset(seed=seed, options=options)

        if self.generate_valid_action_trees:
            self.last_valid_action_trees = self._get_valid_action_trees()

        return self._transform(observation), info

    def step(  # type: ignore
        self, action: Action
    ) -> Tuple[
        Union[List[Observation], Observation],
        Union[List[int], int],
        bool,
        bool,
        Dict[Any, Any],
    ]:
        observation, reward, truncated, done, info = super().step(action)

        if not isinstance(self, MultiAgentEnv):
            extra_info = self._after_step(observation, reward, done, info)
            info.update(extra_info)

        if self.generate_valid_action_trees:
            self.last_valid_action_trees = self._get_valid_action_trees()
            info["valid_action_tree"] = self.last_valid_action_trees.copy()

        self.env_steps += 1

        return self._transform(observation), reward, done, truncated, info

    def render(self) -> Union[str, npt.NDArray]:  # type: ignore
        return super().render()

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
        if not isinstance(self, MultiAgentEnv):
            if self.include_agent_videos:
                self._agent_recorder = ObserverEpisodeRecorder(
                    self, 1, self.video_frequency, self.video_directory, self.fps
                )
            if self.include_global_video:
                self._global_recorder = ObserverEpisodeRecorder(
                    self, "global", self.video_frequency, self.video_directory, self.fps
                )


class RLlibMultiAgentWrapper(RLlibEnv, MultiAgentEnv):
    def __init__(self, env: RLlibEnv) -> None:
        self._player_done_variable = env.env_config.get("player_done_variable", None)

        # Used to keep track of agents that are active in the environment
        self._active_agents: Set[int] = set()

        super().__init__(env.env_config)

        assert (
            self.player_count > 1
        ), "RLlibMultiAgentWrapper can only be used with environments that have multiple agents"

    def _to_multi_agent_map(self, data: List[Any]) -> Dict[int, Any]:
        return {a: data[a - 1] for a in self._active_agents}

    def reset( # type: ignore
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ) -> Tuple[Dict[int, Any], Dict[Any, Any]]:
        obs, info = super().reset(seed, options)
        self._agent_ids = [a for a in range(1, self.player_count + 1)]
        self._active_agents.update([a for a in range(1, self.player_count + 1)])
        return self._to_multi_agent_map(obs), info # type: ignore

    def _resolve_player_done_variable(self) -> bool:
        resolved_variables = self.game.get_global_variables(
            [self._player_done_variable]
        )
        is_player_done = resolved_variables[self._player_done_variable]
        assert isinstance(is_player_done, bool)
        return is_player_done

    def _after_step(
        self,
        obs_map: Dict[Any, Any],
        reward_map: Dict[Any, float],
        done_map: Dict[Any, bool],
        truncated_map: Dict[Any, bool],
        info_map: Dict[str, Any],
    ):
        extra_info = {}

        if self.is_video_enabled():
            videos_list = []
            if self.include_agent_videos:
                for a in self._active_agents:
                    end_video = (
                        done_map[a]
                        or done_map["__all__"]
                        or truncated_map[a]
                        or truncated_map["__all__"]
                    )
                    video_info = self._agent_recorders[a].step(
                        self.level_id, self.env_steps, end_video
                    )
                    if video_info is not None:
                        videos_list.append(video_info)
            if self.include_global_video and self._global_recorder is not None:
                end_video = done_map["__all__"] or truncated_map["__all__"]
                video_info = self._global_recorder.step(
                    self.level_id, self.env_steps, end_video
                )
                if video_info is not None:
                    videos_list.append(video_info)

            self.videos = videos_list

        return extra_info

    def step(self, action_dict: MultiAgentDict):
        actions_array = [None] * self.player_count
        for agent_id, action in action_dict.items():
            actions_array[agent_id - 1] = action

        obs, reward, all_done, all_truncated, info = super().step(actions_array)

        done_map = {"__all__": all_done}
        truncated_map = {"__all__": all_truncated}

        if self._player_done_variable is not None:
            griddly_players_done = self._resolve_player_done_variable()

            for agent_id in self._active_agents:
                done_map[agent_id] = griddly_players_done[agent_id] == 1
                truncated_map[
                    agent_id
                ] = False  # TODO: not sure how to support multi-agent truncated?
        else:
            for p in range(1, self.player_count + 1):
                done_map[p] = False

        if self.generate_valid_action_trees:
            info_map = self._to_multi_agent_map(
                [
                    {"valid_action_tree": valid_action_tree}
                    for valid_action_tree in info["valid_action_tree"]
                ]
            )
        else:
            info_map = self._to_multi_agent_map(defaultdict(dict))

        if self.record_actions:
            for event in info["History"]:
                event_player_id = event["PlayerId"]
                if event_player_id != 0:
                    if "History" not in info_map[event_player_id]:
                        info_map[event_player_id]["History"] = []
                    info_map[event_player_id]["History"].append(event)

        obs_map = self._to_multi_agent_map(obs)
        reward_map = self._to_multi_agent_map(reward)

        # Finally remove any agent ids that are done
        for agent_id, is_done in done_map.items():
            if is_done:
                self._active_agents.discard(agent_id)

        self._after_step(obs_map, reward_map, done_map, truncated_map, info_map)

        return obs_map, reward_map, done_map, truncated_map, info_map

    def is_video_enabled(self):
        return (
            self.record_video_config is not None
            and self._env_idx is not None
            and self._env_idx == 0
        )

    def on_episode_start(self, worker_idx, env_idx):
        self._env_idx = env_idx
        self._worker_idx = worker_idx

        if self.is_video_enabled() and not self.video_initialized:
            self.init_video_recording()
            self.video_initialized = True

    def init_video_recording(self):
        if self.include_agent_videos:
            self._agent_recorders = {}
            for agent_id in self._agent_ids:
                self._agent_recorders[agent_id] = ObserverEpisodeRecorder(
                    self, agent_id - 1, self.video_frequency, self.video_directory
                )
        if self.include_global_video:
            self._global_recorder = ObserverEpisodeRecorder(
                self, "global", self.video_frequency, self.video_directory
            )
