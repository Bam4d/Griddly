from pdb import set_trace as T
import gym
from griddly import GymWrapperFactory, gd
import ray
import ray.rllib.agents.ppo as ppo
from ray.tune.registry import register_env
import copy

reshp = (1, 2, 0)


def unregister():
    for env in copy.deepcopy(gym.envs.registry.env_specs):
        if 'GDY' in env:
            print("Remove {} from registry".format(env))
            del gym.envs.registry.env_specs[env]


class RLLibEnv(gym.Env):
    def __init__(self, config=None):
        self.env = gym.make('GDY-Spider-Nest-v0')
        self.env.reset()
        observation_space = self.env.observation_space
        self.observation_space = gym.spaces.Box(observation_space.low.transpose(reshp),
                                                observation_space.high.transpose(reshp))
        self.action_space = self.env.action_space

        # TODO: necessary step toward having num_workers > 0, but we are still missing python_griddly after this

    #     wrapper = GymWrapperFactory()
    #     wrapper.build_gym_from_yaml(
    #      'Spider-Nest',
    #      'Single-Player/GVGAI/spider-nest.yaml',
    #      player_observer_type=gd.ObserverType.VECTOR,
    #      level=0
    #     )

    def reset(self):
        obs = self.env.reset()
        obs = obs.transpose(reshp)

        return obs

    def step(self, act):
        obs, rew, done, info = self.env.step(act)
        obs = obs.transpose(reshp)
        #     self.render()

        return obs, rew, done, info

    def render(self):
        self.env.render(observer="global")


if __name__ == '__main__':
    unregister()
    wrapper = GymWrapperFactory()
    wrapper.build_gym_from_yaml(
        'Spider-Nest',
        'Single-Player/GVGAI/spider-nest.yaml',
        player_observer_type=gd.ObserverType.VECTOR,
        level=0
    )
    ray.init()
    rllib_config = {
        "framework": "torch",
        "num_workers": 0,
        "num_envs_per_worker": 1,
        "model": {
            # "conv_filters": [[32, (13, 9), 1]],
            "conv_filters": [[32, (7, 7), 1]],
        },
        "env_config": {
            "config": None,
        },

    }

    trainer = ppo.PPOTrainer(config=rllib_config, env=RLLibEnv)

    TEST = False

    if not TEST:
        while True:
            res = trainer.train()
        print(res)

    else:
        env = RLLibEnv()
        env.reset()

        for i in range(1000):
            env.step(env.action_space.sample())
            env.render()