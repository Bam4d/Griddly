import os
import sys

import ray
from ray import tune
from ray.rllib.models import ModelCatalog
from ray.tune.integration.wandb import WandbLoggerCallback
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.environment.core import RLlibEnv
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.torch.agents.conv_agent import SimpleConvAgent
# from griddly.util.rllib.callbacks import GriddlyCallbacks
from griddly.util.rllib.torch.conditional_actions.conditional_action_policy_trainer import \
    ConditionalActionImpalaTrainer

if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    yaml_files = [
        os.path.realpath('clusters_po.yaml'),
        os.path.realpath('clusters_po_with_push.yaml'),
        os.path.realpath('clusters_po_with_push_seperate_colors.yaml')
    ]


    #ray.init(num_gpus=1)
    ray.init(num_gpus=1, local_mode=True)

    env_name = "ray-griddly-env"

    register_env(env_name, RLlibEnv)
    ModelCatalog.register_custom_model("SimpleConv", SimpleConvAgent)

    wandbLoggerCallback = WandbLoggerCallback(
        project='conditional_actions',
        group='baseline',
        api_key_file='~/.wandb_rc'
    )

    max_training_steps = 20000000

    config = {
        'framework': 'torch',
        'num_workers': 8,
        'num_envs_per_worker': 4,

        # 'callbacks': GriddlyCallbacks,

        'model': {
            'custom_model': tune.grid_search(['SimpleConv', 'GAP']),
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            'record_video_config': {
                'frequency': 100000,
                'directory': 'baseline_videos'
            },

            # Put this here so it shows up in wandb
            'generate_valid_action_trees': False,
            'random_level_on_reset': True,
            'yaml_file': tune.grid_search(yaml_files),
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
        ],

    }

    stop = {
        "timesteps_total": max_training_steps,
    }

    result = tune.run(ConditionalActionImpalaTrainer, config=config, stop=stop, callbacks=[wandbLoggerCallback])
