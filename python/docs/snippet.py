
import gym
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    environment_name = 'sokoban'
    level = 2

    wrapper.build_gym_from_yaml(environment_name, f'Single-Player/GVGAI/{environment_name}.yaml', player_observer_type=gd.ObserverType.SPRITE_2D, level=level)

    env = gym.make(f'GDY-{environment_name}-v0')