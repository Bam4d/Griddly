import os
import sys

import ray
from ray import tune
from ray.rllib.models import ModelCatalog
from ray.tune.integration.wandb import WandbLoggerCallback
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.callbacks import GriddlyCallbacks
from griddly.util.rllib.environment.core import RLlibEnv
from griddly.util.rllib.torch.agents.conv_agent import SimpleConvAgent
from griddly.util.rllib.torch.conditional_actions.conditional_action_policy_trainer import \
    ConditionalActionImpalaTrainer

import argparse

parser = argparse.ArgumentParser(description='Run experiments')

parser.add_argument('--yaml-file', help='YAML file condining GDY for the game')

parser.add_argument('--root-directory', default=os.path.expanduser("~/ray_results"),
                    help='root directory for all data associated with the run')
parser.add_argument('--num-gpus', default=1, type=int, help='Number of GPUs to make available')

parser.add_argument('--num-workers', default=8, type=int, help='Number of workers')
parser.add_argument('--num-envs-per-worker', default=2, type=int, help='Number of workers')
parser.add_argument('--num-gpus-per-worker', default=0, type=float, help='Number of gpus per worker')
parser.add_argument('--num-cpus-per-worker', default=1, type=float, help='Number of gpus per worker')
parser.add_argument('--max-training-steps', default=20000000, type=int, help='Number of workers')

parser.add_argument('--capture-video', action='store_true', help='enable video capture')
parser.add_argument('--video-directory', default='videos', help='directory of video')
parser.add_argument('--video-frequency', type=int, default=10000, help='Frequency of videos')

parser.add_argument('--lr', type=float, default=0.0005, help='learning rate')

if __name__ == '__main__':

    args = parser.parse_args()

    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(include_dashboard=False, num_gpus=args.num_gpus)
    # ray.init(include_dashboard=False, num_gpus=args.num_gpus, local_mode=True)

    env_name = "ray-griddly-env"

    register_env(env_name, RLlibEnv)
    ModelCatalog.register_custom_model("SimpleConv", SimpleConvAgent)

    wandbLoggerCallback = WandbLoggerCallback(
        project='conditional_actions',
        api_key_file='~/.wandb_rc',
        dir=os.path.join(args.root_directory, 'wandb')
    )

    max_training_steps = args.max_training_steps

    config = {
        'framework': 'torch',
        'num_workers': args.num_workers,
        'num_envs_per_worker': args.num_envs_per_worker,
        'num_gpus_per_worker': float(args.num_gpus_per_worker),
        'num_cpus_per_worker': args.num_cpus_per_worker,

        'callbacks': GriddlyCallbacks,

        'model': {
            'custom_model': 'SimpleConv',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {

            'allow_nop': tune.grid_search([True, False]),
            'invalid_action_masking': tune.grid_search(['none', 'conditional', 'collapsed']),
            # 'invalid_action_masking': 'collapsed',
            # 'allow_nop': False,
            'generate_valid_action_trees': True,
            'random_level_on_reset': True,
            'yaml_file': args.yaml_file,
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'max_steps': 1000,
        },
        'entropy_coeff_schedule': [
            [0, 0.01],
            [max_training_steps, 0.0]
        ],
        'lr_schedule': [
            [0, args.lr],
            [max_training_steps, 0.0]
        ],

    }

    if args.capture_video:
        config['env_config']['record_video_config'] = {
            'frequency': args.video_frequency,
            'directory': os.path.join(args.root_directory, args.video_directory)
        }

    stop = {
        "timesteps_total": max_training_steps,
    }

    result = tune.run(ConditionalActionImpalaTrainer, local_dir=args.root_directory, config=config, stop=stop,
                      callbacks=[wandbLoggerCallback])
