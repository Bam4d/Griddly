from typing import Tuple

import gym
from gym.spaces import Dict
from ray.rllib import MultiAgentEnv
from ray.rllib.utils.typing import MultiAgentDict

from griddly import GymWrapper
import numpy as np


class ValidActionTreeSpace(gym.spaces.Space):
    """
    Allows possible actions to be passed to the policy as an observation so it can be used in invalid action masking
    """

    def __init__(self, shape=None, dtype=None):
        super().__init__(shape, dtype)

    def sample(self):
        return np.zeros(self.shape)

    def contains(self, x):
        return len(x.shape) == len(self.shape) and \
               x.shape[0] <= self.shape[0] and \
               x.shape[1] == self.shape[1]


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

        super().reset()

        self.set_transform()

    def _transform_obs_space(self, observation_space):

        obs_space_dict = {
            'obs': gym.spaces.Box(
                observation_space.low.transpose((1, 2, 0)),
                observation_space.high.transpose((1, 2, 0)),
                dtype=np.float,
            )
        }

        # To fit and compress the valid action data
        if self._invalid_action_masking:
            # TODO: valid action tree almost certainly can be compressed in a better format than it is currently.
            # RLLib forces these variables to be Tensors, so this will have to do for now

            # Basically contain a list of [x,y,action_type,action_id_mask]
            shape = (
                self.grid_width * self.grid_height * self.action_count,
                3 + self.max_action_ids  # x,y,action_type,action_id_mask
            )

            obs_space_dict['valid_action_tree'] = ValidActionTreeSpace(shape=shape)

        return Dict(obs_space_dict)

    def _get_location_mask_for_player(self, player_id):
        if not self.has_avatar:
            grid_width_mask = np.zeros(self.grid_width)
            grid_height_mask = np.zeros(self.grid_height)

            locations_and_action_names = self.game.get_available_actions(player_id)
            for location, action_names in locations_and_action_names.items():
                grid_width_mask[location[0]] = 1
                grid_height_mask[location[1]] = 1
            return grid_height_mask, grid_width_mask, locations_and_action_names

    def _transform_with_action_mask(self, observation):
        pass
        # return_obs = {}
        #
        # if self.player_count > 0:
        #
        #     for p in self.player_count:
        #         grid_height_mask, grid_width_mask, locations_and_action_names = self._get_location_mask_for_player(p+1)
        #         return_obs['location_action_mask'] = np.concat([grid_height_mask, grid_width_mask])
        #
        #         for location, action_names in locations_and_action_names.items():
        #
        #             if self.action_count > 1:
        #
        # unit_action_mask_parts = []
        # if self.action_count > 1:
        #     unit_action_mask_parts.append(self.action_count)
        #
        # unit_action_mask_parts.append(self.max_action_ids)
        #
        # obs_space_dict['unit_action_mask'] = gym.spaces.Dict(0, 1, shape=unit_action_mask_parts, dtype=np.float)

    def _obs_transform(self, obs):
        return obs.transpose(1, 2, 0).astype(np.float)

    def _transform(self, observation):

        transformed_obs = {}

        if self.player_count > 1:
            transformed_obs['obs'] = [self._obs_transform(obs) for obs in observation['obs']]
        else:
            transformed_obs['obs'] = self._obs_transform(observation['obs'])

        if self._invalid_action_masking:
            transformed_obs['valid_action_tree'] = 

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
        return self._transform(observation), reward, done, info

    def render(self, mode='human', observer=0):
        return super().render(mode, observer='global')


class RLlibMultiAgentWrapper(RLlibWrapper, MultiAgentEnv):

    def __init__(self, env_config):
        super().__init__(**env_config)

        self._player_done_variable = env_config.get('player_done_variable', None)

        assert self.player_count > 1, 'RLlibMultiAgentWrapper can only be used with environments that have multiple agents'

    def _to_rlib_obs(self, obs_array):
        return {p: obs for p, obs in enumerate(obs_array)}

    def reset(self, **kwargs):
        obs = super().reset(**kwargs)
        return self._to_rlib_obs(obs)

    def get_info(self):
        return self.info

    def step(self, action_dict: MultiAgentDict):
        actions_array = np.zeros((self.player_count, *self.action_space.shape))
        for agent_id, action in action_dict.items():
            actions_array[agent_id] = action

        obs, reward, all_done, self.info = super().step(actions_array)

        done = {'__all__': all_done}

        if self._player_done_variable is not None:
            player_done = self.game.get_global_variable([self._player_done_variable])
        else:
            for p in range(self.player_count):
                done[p + 1] = False

        return self._to_rlib_obs(obs), self._to_rlib_obs(reward), done, {}
