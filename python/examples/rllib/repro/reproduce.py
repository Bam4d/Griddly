
import ray
from ray import tune
from ray.rllib.agents.ppo import PPOTrainer
from ray.rllib.examples.env.random_env import RandomEnv

if __name__ == '__main__':

    ray.init(num_gpus=1)

    env_name = 'ray-ma-env'

    config = {
        'framework': 'torch',
        'num_workers': 10,
        #'num_envs_per_worker': 1,

        #'use_gae': False,

        'num_gpus': 1,

        'env': RandomEnv,

    }

    stop = {
        'timesteps_total': 10000,
    }

    trainer = PPOTrainer(config=config)

    for i in range(100):
        trainer.train()

    #result = tune.run(PPOTrainer, config=config, stop=stop)