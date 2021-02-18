import os
import sys

import numpy as np
import ray
from gym.spaces import Box, MultiDiscrete
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.agents.ppo import PPOTrainer
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib import RLlibMultiAgentWrapper
from griddly.util.rllib.torch import GAPAgent

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1, local_mode=True)

    test_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'test_ma_grouped.yaml')

    env_name = 'ray-ma-grouped-env'

    env_config = {
        # in the griddly environment we set a variable to let the training environment
        # know if that player is no longer active
        'player_done_variable': 'player_done',

        # 'record_video_config': {
        #     'frequency': 10000  # number of rollouts
        # },

        'yaml_file': test_path,
        'global_observer_type': gd.ObserverType.SPRITE_2D,
        'level': 0,
        'max_steps': 1000,
    }


    def _env_creator(env_config):
        agent_groups = {
            "groups": {
                "group1": [1, 2, 3],
                "group2": [4, 5, 6],
                "group3": [7, 8, 9],
                "group4": [10, 11, 12],
            }
        }

        obs_space = Box(0, 255, shape=(5, 5, 3), dtype=np.float)
        act_space = MultiDiscrete([2, 5])
        return RLlibMultiAgentWrapper(env_config).with_agent_groups(
            agent_groups,
            obs_space=obs_space,
            act_space=act_space
        )

    register_env(env_name, _env_creator)

    ModelCatalog.register_custom_model('GAP', GAPAgent)

    config = {
        'framework': 'torch',

        'train_batch_size': 256,
        'rollout_fragment_length': 10,
        'lr': 0.0003,

        'num_workers': 8,
        'num_envs_per_worker': 1,
        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': env_config
    }

    stop = {
        'timesteps_total': 2000000,
    }

    result = tune.run(ImpalaTrainer, config=config, stop=stop)
