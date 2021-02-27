from uuid import uuid1
from collections import defaultdict
from enum import Enum
from typing import Tuple

import gym
from gym.spaces import Dict
from ray.rllib import MultiAgentEnv
from ray.rllib.utils.typing import MultiAgentDict

from griddly import GymWrapper
import numpy as np

from griddly.RenderTools import VideoRecorder


class RecordingState(Enum):
    NOT_RECORDING = 1
    WAITING_FOR_EPISODE_START = 2
    BEFORE_RECORDING = 3
    RECORDING = 4


class RLlibWrapper(GymWrapper):
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
        super().__init__(**env_config)

        self._invalid_action_masking = env_config.get('invalid_action_masking', False)
        self._record_video_config = env_config.get('record_video_config', None)

        super().reset()

        self._recording_state = None
        self._env_steps = 0

        if self._record_video_config is not None:
            self._recording_state = RecordingState.BEFORE_RECORDING
            self._record_frequency = self._record_video_config.get('frequency', 1000)

        self.set_transform()

    def _transform_obs_space(self, observation_space):

        return gym.spaces.Box(
            observation_space.low.transpose((1, 2, 0)),
            observation_space.high.transpose((1, 2, 0)),
            dtype=np.float,
        )

    def _get_player_action_tree(self, player_id):

        valid_action_tree = defaultdict(lambda: defaultdict(lambda: defaultdict(defaultdict)))
        for location, action_names in self.game.get_available_actions(player_id).items():
            for action_name, action_ids in self.game.get_available_action_ids(location, list(action_names)).items():
                if len(action_ids) > 0:
                    valid_action_tree[location[0]][location[1]][self.action_names.index(action_name)] = action_ids
        return valid_action_tree

    def _build_valid_action_trees(self):
        player_valid_action_trees = []

        if self.player_count > 0:
            for p in range(self.player_count):
                player_valid_action_trees.append({'valid_action_tree': self._get_player_action_tree(p + 1)})

        else:
            player_valid_action_trees.append({'valid_action_tree': self._get_player_action_tree(1)})

        return player_valid_action_trees

    def _transform(self, observation):

        if self.player_count > 1:
            transformed_obs = [obs.transpose(1, 2, 0).astype(np.float) for obs in observation]
        else:
            transformed_obs = observation.transpose(1, 2, 0).astype(np.float)

        return transformed_obs

    def _after_step(self, observation, reward, done, info):
        if self._recording_state is not None:
            if self._recording_state is RecordingState.NOT_RECORDING and self._env_steps % self._record_frequency == 0:
                self._recording_state = RecordingState.WAITING_FOR_EPISODE_START

            if self._recording_state == RecordingState.BEFORE_RECORDING:
                global_obs = self.render(observer='global', mode='rgb_array')
                self._global_recorder = VideoRecorder()
                self._global_recorder.start(f'global_video_{uuid1()}_{self._env_steps}.mp4', global_obs.shape)
                self._recording_state = RecordingState.RECORDING

            if self._recording_state == RecordingState.RECORDING:
                global_obs = self.render(observer='global', mode='rgb_array')
                self._global_recorder.add_frame(global_obs)
                if done:
                    self._recording_state = RecordingState.NOT_RECORDING
                    self._global_recorder.close()

            if self._recording_state == RecordingState.WAITING_FOR_EPISODE_START:
                if done:
                    self._recording_state = RecordingState.BEFORE_RECORDING

    def set_transform(self):
        """
        Create the transform for rllib based on the observation space
        """

        if self.player_count > 1:
            self.observation_space = self.observation_space[0]
            self.action_space = self.action_space[0]

        self.observation_space = self._transform_obs_space(self.observation_space)

    def reset(self, **kwargs):
        observation = super().reset(**kwargs)
        self.set_transform()
        return self._transform(observation)

    def step(self, action):
        observation, reward, done, info = super().step(action)

        self._after_step(observation, reward, done, info)

        self._env_steps += 1

        return self._transform(observation), reward, done, info

    def render(self, mode='human', observer=0):
        return super().render(mode, observer='global')


class RLlibMultiAgentWrapper(RLlibWrapper, MultiAgentEnv):

    def __init__(self, env_config):
        super().__init__(env_config)

        self._player_done_variable = env_config.get('player_done_variable', None)

        # Used to keep track of agents that are active in the environment
        self._active_agents = set()

        assert self.player_count > 1, 'RLlibMultiAgentWrapper can only be used with environments that have multiple agents'

    def _to_multi_agent_map(self, data):
        return {a: data[a-1] for a in self._active_agents}

    def reset(self, **kwargs):
        obs = super().reset(**kwargs)
        self._active_agents.update([a+1 for a in range(self.player_count)])
        return self._to_multi_agent_map(obs)

    def _resolve_player_done_variable(self):
        resolved_variables = self.game.get_global_variable([self._player_done_variable])
        return resolved_variables[self._player_done_variable]

    def step(self, action_dict: MultiAgentDict):
        actions_array = np.zeros((self.player_count, *self.action_space.shape))
        for agent_id, action in action_dict.items():
            actions_array[agent_id-1] = action

        obs, reward, all_done, _ = super().step(actions_array)

        done_map = {'__all__': all_done}

        if self._player_done_variable is not None:
            griddly_players_done = self._resolve_player_done_variable()

            for agent_id in self._active_agents:
                done_map[agent_id] = griddly_players_done[agent_id] == 1 or all_done
        else:
            for p in range(self.player_count):
                done_map[p] = False

        if self._invalid_action_masking:
            info_map = self._to_multi_agent_map(self._build_valid_action_trees())
        else:
            info_map = self._to_multi_agent_map(defaultdict(dict))

        obs_map = self._to_multi_agent_map(obs)
        reward_map = self._to_multi_agent_map(reward)

        # Finally remove any agent ids that are done
        for agent_id, is_done in done_map.items():
            if is_done:
                self._active_agents.discard(agent_id)

        assert len(obs_map) == len(reward_map)
        assert len(obs_map) == len(done_map)-1
        assert len(obs_map) == len(info_map)


        return obs_map, reward_map, done_map, info_map