import gym
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        'Sokoban',
        'Single-Player/GVGAI/sokoban.yaml',
        player_observer_type=gd.ObserverType.VECTOR,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=2
    )

    env = gym.make(f'GDY-Sokoban-v0')
    env.reset()

    env.render(observer='global')