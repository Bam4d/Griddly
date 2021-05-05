import os
import sys

import ray
from ray import tune
from ray.rllib.models import ModelCatalog
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.callbacks import ActionTrackerCallback
from griddly.util.rllib.environment.core import RLlibMultiAgentWrapper, RLlibEnv
from griddly.util.rllib.torch.agents.impala_cnn import ImpalaCNNAgent
from griddly.util.rllib.torch.conditional_actions.conditional_action_policy_trainer import \
    ConditionalActionImpalaTrainer

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = "griddly-rts-env"


    def _create_env(env_config):
        env = RLlibEnv(env_config)
        return RLlibMultiAgentWrapper(env, env_config)


    register_env(env_name, _create_env)
    ModelCatalog.register_custom_model("ImpalaCNN", ImpalaCNNAgent)

    max_training_steps = 100000000

    config = {
        'framework': 'torch',

        'num_workers': 8,
        'num_envs_per_worker': 5,

        'callbacks': ActionTrackerCallback,

        'model': {
            'custom_model': 'ImpalaCNN',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            'generate_valid_action_trees': True,
            'yaml_file': 'RTS/GriddlyRTS.yaml',
            'global_observer_type': gd.ObserverType.ISOMETRIC,
            'level': 0,
            'record_actions': True,
            'max_steps': 1000,

            'record_video_config': {
                'frequency': 20000,  # number of rollouts
                'directory': 'videos'
            },
        },
        'entropy_coeff_schedule': [
            [0, 0.001],
            [max_training_steps, 0.0]
        ],
        'lr_schedule': [
            [0, 0.0005],
            [max_training_steps, 0.0]
        ],

    }

    stop = {
        "timesteps_total": max_training_steps,
    }

    result = tune.run(ConditionalActionImpalaTrainer, config=config, stop=stop)
