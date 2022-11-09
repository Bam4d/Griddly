import os
from collections import defaultdict

import gym
import numpy as np
from ray.rllib import MultiAgentEnv
from ray.rllib.utils.typing import MultiAgentDict

from griddly import GymWrapper
from griddly.util.rllib.environment.observer_episode_recorder import (
    ObserverEpisodeRecorder,
)


class _RLlibEnvCache:
    def __init__(self):
        self.action_space = None
        self.observation_space = None

    def reset(self):
        self.__init__()


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

    def __init__(self, env_config):
        self._rllib_cache = _RLlibEnvCache()
        super().__init__(**env_config)

        self.env_config = env_config


        self.env_steps = 0
        self._env_idx = None
        self._worker_idx = None

        self.video_initialized = False

        self.record_video_config = env_config.get("record_video_config", None)

        self.videos = []

        if self.record_video_config is not None:
            self.video_frequency = self.record_video_config.get("frequency", 1000)
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

    def _transform(self, observation):

        if self.player_count > 1:
            transformed_obs = [
                obs.transpose(1, 2, 0).astype(float) for obs in observation
            ]
        elif isinstance(observation, dict):
            transformed_obs = {
                k: v.transpose(1, 2, 0).astype(float) for k, v in observation.items()
            }
        else:
            transformed_obs = observation.transpose(1, 2, 0).astype(float)

        return transformed_obs

    def _after_step(self, observation, reward, done, info):
        extra_info = {}

        if self.is_video_enabled():
            videos_list = []
            if self.include_agent_videos:
                video_info = self._agent_recorder.step(
                    self.level_id, self.env_steps, done
                )
                if video_info is not None:
                    videos_list.append(video_info)
            if self.include_global_video:
                video_info = self._global_recorder.step(
                    self.level_id, self.env_steps, done
                )
                if video_info is not None:
                    videos_list.append(video_info)

            self.videos = videos_list

        return extra_info

    @property
    def action_space(self):
        if self._rllib_cache.action_space is None:
            self._rllib_cache.action_space = super().action_space[0] if self.player_count > 1 else super().action_space
        return self._rllib_cache.action_space

    @property
    def observation_space(self):
        if self._rllib_cache.observation_space is None:
            obs_space = super().observation_space[0] if self.player_count > 1 else super().observation_space
            self._rllib_cache.observation_space = gym.spaces.Box(
                obs_space.low.transpose((1, 2, 0)).astype(float),
                obs_space.high.transpose((1, 2, 0)).astype(float),
                dtype=float,
            )
        return self._rllib_cache.observation_space

    @observation_space.setter
    def observation_space(self, value):
        self._rllib_cache.observation_space = value

    @action_space.setter
    def action_space(self, value):
        self._rllib_cache.action_space = value

    @property
    def width(self):
        return self.observation_space.shape[0]

    @property
    def height(self):
        return self.observation_space.shape[1]

    def _get_valid_action_trees(self):
        valid_action_trees = self.game.build_valid_action_trees()
        if self.player_count == 1:
            return valid_action_trees[0]
        return valid_action_trees

    def reset(self, **kwargs):

        if self._level_generator is not None:
            kwargs["level_string"] = self._level_generator.generate()
        elif self._random_level_on_reset:
            kwargs["level_id"] = np.random.choice(self.level_count)

        self._rllib_cache.reset()
        observation = super().reset(**kwargs)

        if self.generate_valid_action_trees:
            self.last_valid_action_trees = self._get_valid_action_trees()

        return self._transform(observation)

    def step(self, action):
        observation, reward, done, info = super().step(action)

        if not isinstance(self, MultiAgentEnv):
            extra_info = self._after_step(observation, reward, done, info)
            info.update(extra_info)

        if self.generate_valid_action_trees:
            self.last_valid_action_trees = self._get_valid_action_trees()
            info["valid_action_tree"] = self.last_valid_action_trees.copy()

        self.env_steps += 1

        return self._transform(observation), reward, done, info

    def render(self, mode="human", observer=0):
        return super().render(mode, observer=observer)

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
        if self.player_count == 1:
            if self.include_agent_videos:
                self._agent_recorder = ObserverEpisodeRecorder(
                    self, 1, self.video_frequency, self.video_directory
                )
            if self.include_global_video:
                self._global_recorder = ObserverEpisodeRecorder(
                    self, "global", self.video_frequency, self.video_directory
                )


class RLlibMultiAgentWrapper(RLlibEnv, MultiAgentEnv):
    def __init__(self, env):

        self._player_done_variable = env.env_config.get("player_done_variable", None)

        # Used to keep track of agents that are active in the environment
        self._active_agents = set()

        self._agent_recorders = None
        self._global_recorder = None

        self._worker_idx = None
        self._env_idx = None

        super().__init__(env.env_config)

        assert (
                self.player_count > 1
        ), "RLlibMultiAgentWrapper can only be used with environments that have multiple agents"

    def _to_multi_agent_map(self, data):
        return {a: data[a-1] for a in self._active_agents}

    def reset(self, **kwargs):
        obs = super().reset(**kwargs)
        self._agent_ids = [a for a in range(1, self.player_count+1)]
        self._active_agents.update([a for a in range(1, self.player_count+1)])
        return self._to_multi_agent_map(obs)

    def _resolve_player_done_variable(self):
        resolved_variables = self.game.get_global_variable([self._player_done_variable])
        return resolved_variables[self._player_done_variable]

    def _after_step(self, obs_map, reward_map, done_map, info_map):
        extra_info = {}

        if self.is_video_enabled():
            videos_list = []
            if self.include_agent_videos:
                for a in self._active_agents:
                    video_info = self._agent_recorders[a].step(
                        self.level_id, self.env_steps, done_map[a]
                    )
                    if video_info is not None:
                        videos_list.append(video_info)
            if self.include_global_video:
                video_info = self._global_recorder.step(
                    self.level_id, self.env_steps, done_map["__all__"]
                )
                if video_info is not None:
                    videos_list.append(video_info)

            self.videos = videos_list

        return extra_info

    def step(self, action_dict: MultiAgentDict):
        actions_array = [None] * self.player_count
        for agent_id, action in action_dict.items():
            actions_array[agent_id-1] = action

        obs, reward, all_done, info = super().step(actions_array)

        done_map = {"__all__": all_done}

        if self._player_done_variable is not None:
            griddly_players_done = self._resolve_player_done_variable()

            for agent_id in self._active_agents:
                done_map[agent_id] = griddly_players_done[agent_id] == 1
        else:
            for p in range(1, self.player_count+1):
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

        self._after_step(obs_map, reward_map, done_map, info_map)

        return obs_map, reward_map, done_map, info_map

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
                    self, agent_id, self.video_frequency, self.video_directory
                )
        if self.include_global_video:
            self._global_recorder = ObserverEpisodeRecorder(
                self, "global", self.video_frequency, self.video_directory
            )
