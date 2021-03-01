import os
import sys

import ray
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.wrappers.core import RLlibMultiAgentWrapper, RLlibEnv

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    test_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'test_ma_tag_coins.yaml')

    env_name = 'ray-ma-grouped-env'

    # Create the gridnet environment and wrap it in a multi-agent wrapper for self-play
    def _create_env(env_config):
        env = RLlibEnv(env_config)
        return RLlibMultiAgentWrapper(env, env_config)

    register_env(env_name, _create_env)

    ModelCatalog.register_custom_model('GAP', GAPAgent)

    config = {
        'framework': 'torch',
        'num_workers': 6,
        'num_envs_per_worker': 1,

        'train_batch_size': 2048,

        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            # in the griddly environment we set a variable to let the training environment
            # know if that player is no longer active
            'player_done_variable': 'player_done',

            'record_video_config': {
                'frequency': 10000  # number of rollouts
            },

            'yaml_file': test_path,
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'level': 2,
            'max_steps': 1000,
        },
        'lr': tune.grid_search([0.0001, 0.0005, 0.001, 0.005])
    }

    stop = {
        'timesteps_total': 2000000,
    }

    result = tune.run(ImpalaTrainer, config=config, stop=stop)
