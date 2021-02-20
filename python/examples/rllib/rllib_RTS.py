import os
import sys

import ray
from ray import tune
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib import RLlibMultiAgentWrapper
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.torch.impala.impala import InvalidActionMaskingImpalaTrainer


if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = 'ray-griddly-rts-env'

    register_env(env_name, RLlibMultiAgentWrapper)
    ModelCatalog.register_custom_model('GAP', GAPAgent)

    config = {
        'framework': 'torch',
        'num_workers': 11,
        'num_envs_per_worker': 1,

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
                'frequency': 10000  # number of rollouts
            },

            'yaml_file': 'RTS/GriddlyRTS.yaml',
            'global_observer_type': gd.ObserverType.ISOMETRIC,
            'level': 0,
            'max_steps': 1000,
        },
    }

    stop = {
        'timesteps_total': 20000000,
    }

    result = tune.run(InvalidActionMaskingImpalaTrainer, config=config, stop=stop)
