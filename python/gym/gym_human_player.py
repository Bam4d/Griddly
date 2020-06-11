import os
import sys

import gym
from gym.utils.play import play

from griddly import GymWrapperFactory, gd

sys.path.extend([os.path.join(os.getcwd(), 'Release/bin')])


if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    environment_name = 'zenpuzzle'
    level = 3

    wrapper.build_gym_from_yaml(environment_name, f'single-player/{environment_name}.yaml', player_observer_type=gd.ObserverType.BLOCK_2D, level=level)

    play(gym.make(f'GDY-{environment_name}-v0'), fps=10, zoom=2)
