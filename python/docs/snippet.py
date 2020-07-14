import gym
import numpy as np
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        "ExampleEnv",
        'Single-Player/Mini-Grid/minigrid-drunkdwarf.yaml',
        level=0
    )

    env = gym.make('GDY-ExampleEnv-v0')
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):
        obs, reward, done, info = env.step(env.action_space.sample())
        env.render()

        env.render(observer='global')