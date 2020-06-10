import os
import sys

import gym
from gym.utils.play import play

from griddly_python import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    environment_name = 'sokoban'
    level = 2

    wrapper.build_gym_from_yaml(environment_name, f'single-player/{environment_name}.yaml', player_observer_type=gd.ObserverType.SPRITE_2D, level=level)

    play(gym.make(f'GDY-{environment_name}-v0'), fps=10, zoom=2)
