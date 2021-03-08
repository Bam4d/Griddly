import os
import sys

import ray
from ray import tune
from ray.rllib.agents.ppo import PPOTrainer
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.torch.agents.conv_agent import SimpleConvAgent
from griddly.util.rllib.wrappers.core import RLlibMultiAgentWrapper, RLlibEnv

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = 'ray-ma-env'

    # Create the gridnet environment and wrap it in a multi-agent wrapper for self-play
    def _create_env(env_config):
        env = RLlibEnv(env_config)
        return RLlibMultiAgentWrapper(env, env_config)

    register_env(env_name, _create_env)

    ModelCatalog.register_custom_model('SimpleConv', SimpleConvAgent)

    max_training_steps = 50000000

    config = {
        'framework': 'torch',
        'num_workers': 8,
        'num_envs_per_worker': 2,

        'num_gpus': 1,

        'model': {
            'custom_model': 'SimpleConv',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            # in the griddly environment we set a variable to let the training environment
            # know if that player is no longer active
            # 'player_done_variable': 'player_done',

            'record_video_config': {
                'frequency': 20000  # number of rollouts
            },

            'random_level_on_reset': True,
            'yaml_file': 'Multi-Agent/foragers.yaml',
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'max_steps': 500,
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
        'timesteps_total': max_training_steps,
    }

    result = tune.run(PPOTrainer, config=config, stop=stop)
