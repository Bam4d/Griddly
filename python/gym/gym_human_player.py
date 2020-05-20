import os
import sys

import gym
from gym.utils.play import play

sys.path.extend([os.path.join(os.getcwd(), 'Release/bin')])

from tools.GymWrapper import GymWrapperFactory

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    environment_name = 'zenpuzzle'
    level = 4

    wrapper.build_gym_from_yaml(environment_name, f'resources/games/single-player/{environment_name}.yaml', level=level)

    play(gym.make(f'GDY-{environment_name}-v0'), fps=10, zoom=2)
