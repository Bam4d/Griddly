import gym
import numpy as np
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        'Spiders-Adv',
        'Single-Player/Mini-Grid/minigrid-spiders.yaml',
        level=0,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        player_observer_type=gd.ObserverType.SPRITE_2D,
        tile_size=100
    )

    env = gym.make('GDY-Spiders-Adv-v0')
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):
        obs, reward, done, info = env.step(env.action_space.sample())
        env.render()

        env.render(observer='global')