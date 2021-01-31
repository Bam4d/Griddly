import gym
from gym.wrappers import TransformObservation
from ray.rllib.agents.impala import ImpalaTrainer

from griddly import gd, GymWrapper
import ray
from ray.tune.registry import register_env

env_name = "ray-griddly-env"

def env_creator(env_config):
    env = GymWrapper(**env_config)
    env.reset()
    print(env.observation_space.shape)
    env.observation_space = gym.spaces.Box(
        env.observation_space.low.transpose((1, 2, 0)),
        env.observation_space.high.transpose((1, 2, 0))
    )
    env = TransformObservation(env, lambda obs: obs.transpose(1, 2, 0))
    print(env.observation_space.shape)
    return env


register_env(env_name, env_creator)

if __name__ == '__main__':

    ray.init(num_gpus=2)
    rllib_config = {
        "framework": "torch",
        "num_workers": 12,
        "num_envs_per_worker": 3,
        "model": {
            # "conv_filters": [[32, (13, 9), 1]],
            "conv_filters": [
                [64, (3, 3), 1],
                [32, (3, 3), 1],
                [32, (28,11),1]
                # [32, (3, 3), 1]
            ],
            # 'fcnet_hiddens': [256, 128, 64],
            # 'use_lstm': True,
            # 'max_seq_len': 32
        },
        "env_config": {
            'yaml_file': 'Single-Player/GVGAI/butterflies.yaml',
            "level": 6,
            "max_steps": 1000,
        },

    }

    trainer = ImpalaTrainer(rllib_config, env=env_name)

    log_string = "{:3d} reward {:6.2f}/{:6.2f}/{:6.2f} len {:6.2f}. Total Trained: {:6.2f}"

    for s in range(10000):
        result = trainer.train()
        print(
            log_string.format(
                s + 1,
                result["episode_reward_min"],
                result["episode_reward_mean"],
                result["episode_reward_max"],
                result["episode_len_mean"],
                result["info"]["num_steps_trained"],
            )
        )
