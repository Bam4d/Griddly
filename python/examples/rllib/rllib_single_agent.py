import os
import sys

import ray
from griddly.util.rllib.callbacks import VideoCallbacks
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.environment.core import RLlibEnv

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = "ray-griddly-env"

    register_env(env_name, RLlibEnv)
    ModelCatalog.register_custom_model("GAP", GAPAgent)

    max_training_steps = 100000000

    config = {
        'framework': 'torch',
        'num_workers': 8,
        'num_envs_per_worker': 4,

        'callbacks': VideoCallbacks,

        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {

            'record_video_config': {
                'frequency': 100000,
                'directory': 'videos'
            },

            'random_level_on_reset': True,
            'yaml_file': 'Single-Player/GVGAI/clusters_partially_observable.yaml',
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'max_steps': 1000,
        },
        'entropy_coeff_schedule': [
            [0, 0.01],
            [max_training_steps, 0.0]
        ],
        'lr_schedule': [
            [0, 0.0005],
            [max_training_steps, 0.0]
        ]
    }

    stop = {
        "timesteps_total": max_training_steps,
    }

    result = tune.run(ImpalaTrainer, config=config, stop=stop)
