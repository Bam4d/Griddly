import gym
from gym.utils.play import play

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import RenderWindow


def callback(env):
    render_window = RenderWindow(800, 800)

    def _callback(prev_obs, obs, action, rew, env_done, info):
        render_window.render(env.render(observer='global', mode="rgb_array").swapaxes(0,2))
        if rew != 0:
            print(f'Reward: {rew}')
        if env_done:
            print(f'Done!')

    return _callback


if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    # environment_name = 'Mini-Grid/minigrid-drunkdwarf'
    environment_name = 'Mini-Grid/minigrid-spiders'
    # environment_name = 'GVGAI/clusters'
    # environment_name = 'GVGAI/labyrinth_partially_observable'
    level = 4

    wrapper.build_gym_from_yaml(environment_name, f'Single-Player/{environment_name}.yaml',
                                player_observer_type=gd.ObserverType.BLOCK_2D,
                                global_observer_type=gd.ObserverType.SPRITE_2D, level=level, tile_size=50)
    env = gym.make(f'GDY-{environment_name}-v0')
    play(env, callback=callback(env), fps=10, zoom=3)
