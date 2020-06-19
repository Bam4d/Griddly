import gym
from gym.utils.play import play

from griddly import GymWrapperFactory, gd


def callback(env):
    def _callback(prev_obs, obs, action, rew, env_done, info):
        env.render(observer='global')

    return _callback

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    environment_name = 'cookmepasta'
    level = 0

    wrapper.build_gym_from_yaml(environment_name, f'Single-Player/GVGAI/{environment_name}.yaml', player_observer_type=gd.ObserverType.BLOCK_2D, global_observer_type=gd.ObserverType.SPRITE_2D, level=level)
    env = gym.make(f'GDY-{environment_name}-v0')
    play(env, callback=callback(env), fps=10, zoom=3)
