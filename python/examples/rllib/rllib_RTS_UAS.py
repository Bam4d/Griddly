import os
import sys

import ray
from ray import tune
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.torch.impala.impala import InvalidActionMaskingImpalaTrainer
from griddly.util.rllib.wrappers.core import RLlibEnv, RLlibMultiAgentWrapper

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = 'ray-griddly-rts-env'

    test_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'GriddlyRTS_test.yaml')

    def _env_create(env_config):
        env = RLlibEnv(env_config)
        return RLlibMultiAgentWrapper(env, env_config)

    register_env(env_name, _env_create)
    ModelCatalog.register_custom_model('GAP', GAPAgent)

    config = {
        'framework': 'torch',
        'num_workers': 8,
        'num_envs_per_worker': 4,

        # Must be set to false to use the InvalidActionMaskingPolicyMixin
        "_use_trajectory_view_api": False,
        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            # Tell the RLlib wrapper to use invalid action masking
            'invalid_action_masking': True,

            'record_video_config': {
                'frequency': 100000  # number of rollouts
            },

            'yaml_file': test_path,
            'global_observer_type': gd.ObserverType.ISOMETRIC,
            'level': 1,
            'max_steps': 1000,
        },
        'lr': tune.grid_search([0.0001, 0.0005, 0.001, 0.005])
    }

    stop = {
        'timesteps_total': 200000000,
    }

    result = tune.run(InvalidActionMaskingImpalaTrainer, config=config, stop=stop)
