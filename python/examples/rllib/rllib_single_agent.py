import ray
from ray.rllib.agents.impala import ImpalaTrainer
from ray.tune.registry import register_env
from ray.rllib.models import ModelCatalog

from examples.rllib.global_average_pooling_agent import GAPAgent
from griddly import gd
from griddly.util.wrappers.rllib_wrapper import RLlibWrapper

env_name = "ray-griddly-env"

register_env(env_name, RLlibWrapper)
ModelCatalog.register_custom_model("GAP", GAPAgent)

if __name__ == '__main__':

    ray.init()
    rllib_config = {
        'framework': 'torch',
        'num_workers': 12,
        'num_envs_per_worker': 3,
        'monitor': True,
        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env_config': {
            'yaml_file': 'Single-Player/GVGAI/random_butterflies.yaml',
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'level': 6,
            'max_steps': 1000,
        }
    }

    trainer = ImpalaTrainer(rllib_config, env=env_name)

    for s in range(10000):
        result = trainer.train()
        print(
            f'Reward (min/mean/max): {result["episode_reward_min"]:.2f}/{result["episode_reward_mean"]:.2f}/{result["episode_reward_max"]:.2f}. '
            f'Mean Episode Length: {result["episode_len_mean"]:.2f}. '
            f'Total Trained Steps: {result["info"]["num_steps_trained"]}'
        )
