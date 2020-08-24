import gym
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        'Sokoban-Adv',
        'Single-Player/GVGAI/sokoban.yaml',
        player_observer_type=gd.ObserverType.SPRITE_2D,
        level=2
    )

    env = gym.make('GDY-Sokoban-Adv-v0')
    env.reset()