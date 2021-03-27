import argparse
import os
import sys

import gym
import numpy as np
import ray
import torch
from ray.rllib.models.torch.torch_modelv2 import TorchModelV2
from torch import nn
from gym.spaces import MultiDiscrete, Dict, Box
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.models import ModelCatalog
from ray.tune.integration.wandb import WandbLoggerCallback
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib.callbacks import GriddlyCallbacks
from griddly.util.rllib.environment.core import RLlibEnv
from griddly.util.rllib.torch.agents.common import layer_init
from griddly.util.rllib.torch.agents.conv_agent import SimpleConvAgent

parser = argparse.ArgumentParser(description='Run experiments')

parser.add_argument('--yaml-file', help='YAML file condining GDY for the game')

parser.add_argument('--root-directory', default=os.path.expanduser("~/ray_results"),
                    help='root directory for all data associated with the run')
parser.add_argument('--num-gpus', default=1, type=int, help='Number of GPUs to make available to ray.')
parser.add_argument('--num-cpus', default=8, type=int, help='Number of CPUs to make available to ray.')

parser.add_argument('--num-workers', default=7, type=int, help='Number of workers')
parser.add_argument('--num-envs-per-worker', default=5, type=int, help='Number of workers')
parser.add_argument('--num-gpus-per-worker', default=0, type=float, help='Number of gpus per worker')
parser.add_argument('--num-cpus-per-worker', default=1, type=float, help='Number of gpus per worker')
parser.add_argument('--max-training-steps', default=20000000, type=int, help='Number of workers')

parser.add_argument('--capture-video', action='store_true', help='enable video capture')
parser.add_argument('--video-directory', default='videos', help='directory of video')
parser.add_argument('--video-frequency', type=int, default=1000000, help='Frequency of videos')

parser.add_argument('--seed', type=int, default=69420, help='seed for experiments')

parser.add_argument('--lr', type=float, default=0.0005, help='learning rate')


class FlatActionWrapper(gym.Wrapper):

    def __init__(self, env):
        super().__init__(env)

        self._num_action_parts = 1
        self._action_params_offset = 0
        if not self.has_avatar:
            self._num_action_parts += 1
            self._action_params_offset = 1


        self._action_splits = np.zeros(self._num_action_parts)

        self._total_position_params = 0
        if not self.has_avatar:
            self._action_splits[0] = self.width*self.height
            self._total_position_params += self.width*self.height

        self._action_logit_offsets = {}

        total_action_params = 0
        for i, action_name in enumerate(self.env.action_names):
            self._action_logit_offsets[action_name] = total_action_params + self._total_position_params
            total_action_params += self.num_action_ids[action_name]

        self._action_splits[self._action_params_offset] = total_action_params

        self._total_actions = int(np.sum(self._action_splits))

        self.action_space = MultiDiscrete(self._action_splits)
        self.observation_space = Dict({
            'obs': self.observation_space,
            'mask': Box(0, 1, shape=(self._total_actions,)),
        })

    def _get_flat_mask(self):
        flat_mask = np.zeros(self._total_actions)
        for location, action_names in self.env.game.get_available_actions(1).items():
            if not self.has_avatar:
                flat_location = self.width * location[1] + location[0]
                flat_mask[flat_location] = 1
            for action_name, action_ids in self.env.game.get_available_action_ids(location, list(action_names)).items():
                mask_offset = self._action_logit_offsets[action_name]
                flat_mask[mask_offset:mask_offset + self.num_action_ids[action_name]][action_ids] = 1
        return flat_mask

    def _to_griddly_action(self, action):
        # convert the flat action back to Griddly's tree based format

        griddly_action = []
        action_ptr = 0
        if not self.has_avatar:
            x = action[action_ptr] % self.width
            griddly_action.append(x)
            y = int(action[action_ptr] / self.width)
            griddly_action.append(y)
            action_ptr += 1

        if self.action_count > 0:
            action_type_id = 0
            action_param_id = 0
            for action_name in self.action_names:
                action_offset_after_position = (self._action_logit_offsets[action_name] - self._total_position_params)
                next_offset = action_offset_after_position + self.num_action_ids[action_name]
                if next_offset > action[action_ptr]:
                    action_param_id = action[action_ptr] - action_offset_after_position
                    break
                action_type_id += 1

            griddly_action.append(action_type_id)
            griddly_action.append(action_param_id)
        else:
            griddly_action.append(action[action_ptr])

        return griddly_action

    def reset(self, **kwargs):

        obs = super().reset(**kwargs)

        observations = {
            'obs': obs,
            'mask': self._get_flat_mask()
        }

        return observations

    def step(self, action):
        griddly_action = self._to_griddly_action(action)

        obs, reward, info, done = super().step(griddly_action)

        observations = {
            'obs': obs,
            'mask': self._get_flat_mask()
        }

        return observations, reward, info, done


class SimpleConvFlatAgent(TorchModelV2, nn.Module):

    def __init__(self, obs_space, action_space, num_outputs, model_config, name):
        super().__init__(obs_space, action_space, num_outputs, model_config, name)
        nn.Module.__init__(self)

        self._num_objects = obs_space.original_space['obs'].shape[2]
        self._num_actions = num_outputs

        linear_flatten = np.prod(obs_space.original_space['obs'].shape[:2]) * 64

        self.network = nn.Sequential(
            layer_init(nn.Conv2d(self._num_objects, 32, 3, padding=1)),
            nn.ReLU(),
            layer_init(nn.Conv2d(32, 64, 3, padding=1)),
            nn.ReLU(),
            nn.Flatten(),
            layer_init(nn.Linear(linear_flatten, 1024)),
            nn.ReLU(),
            layer_init(nn.Linear(1024, 512)),
            nn.ReLU(),
        )

        self._actor_head = nn.Sequential(
            layer_init(nn.Linear(512, 256), std=0.01),
            nn.ReLU(),
            layer_init(nn.Linear(256, self._num_actions), std=0.01)
        )

        self._critic_head = nn.Sequential(
            layer_init(nn.Linear(512, 1), std=0.01)
        )

    def forward(self, input_dict, state, seq_lens):
        obs_transformed = input_dict['obs']['obs'].permute(0, 3, 1, 2)
        mask = input_dict['obs']['mask']
        network_output = self.network(obs_transformed)
        value = self._critic_head(network_output)
        self._value = value.reshape(-1)
        logits = self._actor_head(network_output)

        logits += torch.maximum(torch.log(mask), torch.tensor(torch.finfo().min))

        return logits, state

    def value_function(self):
        return self._value


if __name__ == '__main__':

    args = parser.parse_args()

    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(include_dashboard=False, num_gpus=args.num_gpus, num_cpus=args.num_cpus)
    #ray.init(include_dashboard=False, num_gpus=1, num_cpus=args.num_cpus, local_mode=True)
    env_name = "ray-griddly-env"


    def _create_env(env_config):
        env = RLlibEnv(env_config)
        return FlatActionWrapper(env)


    register_env(env_name, _create_env)
    ModelCatalog.register_custom_model("SimpleConv", SimpleConvFlatAgent)

    wandbLoggerCallback = WandbLoggerCallback(
        project='conditional_actions',
        api_key_file='~/.wandb_rc',
        dir=args.root_directory
    )

    max_training_steps = args.max_training_steps

    config = {
        'framework': 'torch',
        'seed': args.seed,
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
            'generate_valid_action_trees': False,
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
        real_video_frequency = int(args.video_frequency / (args.num_envs_per_worker * args.num_workers))
        config['env_config']['record_video_config'] = {
            'frequency': real_video_frequency,
            'directory': os.path.join(args.root_directory, args.video_directory)
        }

    stop = {
        "timesteps_total": max_training_steps,
    }

    result = tune.run(ImpalaTrainer, local_dir=args.root_directory, config=config, stop=stop,
                      callbacks=[wandbLoggerCallback])
