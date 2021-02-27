import os
import sys

import ray
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib import RLlibWrapper
from griddly.util.rllib.torch import GAPAgent

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1, local_mode=True)

    env_name = "ray-griddly-env"

    register_env(env_name, RLlibWrapper)
    ModelCatalog.register_custom_model("GAP", GAPAgent)

    config = {
        'framework': 'torch',
        'num_workers': 1,
        'num_envs_per_worker': 1,

        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            # Uncomment this line to apply invalid action masking
            # 'record_video_config': {
            #     'frequency': 10000
            # },

            'yaml_file': 'Single-Player/GVGAI/clusters_partially_observable.yaml',
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'level': 3,
            'max_steps': 1000,
        },
        #'lr': tune.grid_search([0.0001, 0.0005, 0.001, 0.005])
    }

    stop = {
        # "training_iteration": 100,
        "timesteps_total": 5000000,
    }

    result = tune.run(ImpalaTrainer, config=config, stop=stop)
