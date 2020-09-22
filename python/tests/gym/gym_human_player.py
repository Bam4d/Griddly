import gym
from gym.utils.play import play

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import RenderWindow, VideoRecorder


def callback(env):
    render_window = RenderWindow(1104, 584)

    global_obs = env.render(observer='global', mode="rgb_array").swapaxes(0, 2)
    observation_shape = global_obs.shape
    recorder = VideoRecorder()
    recorder.start("human_player_video_test.mp4", observation_shape)

    def _callback(prev_obs, obs, action, rew, env_done, info):

        global_obs = env.render(observer='global', mode="rgb_array")
        recorder.add_frame(global_obs.swapaxes(0, 2))
        render_window.render(global_obs.swapaxes(0, 2))
        if rew != 0:
            print(f'Reward: {rew}')
        if env_done:
            print(f'Done!')
            recorder.close()

        if len(info) > 0:
            print(info)

    return _callback


if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    # environment_name = 'GVGAI/bait_keys'
    # environment_name = 'Mini-Grid/minigrid-drunkdwarf'
    #environment_name = 'Mini-Grid/minigrid-spiders'
    environment_name = 'GVGAI/spider-nest'
    # environment_name = 'GVGAI/clusters'
    # environment_name = 'GVGAI/sokoban2'
    # environment_name = 'GVGAI/labyrinth_partially_observable'
    level = 0

    wrapper.build_gym_from_yaml(environment_name, f'Single-Player/{environment_name}.yaml',
                                player_observer_type=gd.ObserverType.ISOMETRIC,
                                global_observer_type=gd.ObserverType.ISOMETRIC, level=level)
    env = gym.make(f'GDY-{environment_name}-v0')
    env.reset()
    play(env, callback=callback(env), fps=10, zoom=3)
