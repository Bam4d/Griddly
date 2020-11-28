import gym
from gym.utils.play import play

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import RenderWindow, VideoRecorder


def callback(env):
    render_window = RenderWindow(2208, 1168)

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
            #recorder.close()

        if len(info) > 0:
            print(info)

    return _callback


if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    environment_name = 'TestEnv'

    # yaml_path = 'Single-Player/GVGAI/bait_keys.yaml'
    # yaml_path = 'Single-Player/Mini-Grid/minigrid-drunkdwarf.yaml'
    # yaml_path = 'Single-Player/Mini-Grid/minigrid-spiders.yaml'
    # yaml_path = 'Single-Player/GVGAI/spider-nest.yaml'
    # yaml_path = 'Single-Player/GVGAI/cookmepasta.yaml'
    # yaml_path = 'Single-Player/GVGAI/clusters.yaml'
    # yaml_path = 'Single-Player/GVGAI/zenpuzzle.yaml'
    # yaml_path = 'Single-Player/GVGAI/sokoban2.yaml'
    # yaml_path = 'Single-Player/GVGAI/sokoban2_partially_observable.yaml'
    # yaml_path = 'Single-Player/GVGAI/cookmepasta_partially_observable.yaml'
    # yaml_path = 'Single-Player/GVGAI/clusters_partially_observable.yaml'
    # yaml_path = 'Single-Player/GVGAI/bait_partially_observable.yaml'
    # yaml_path = 'Single-Player/GVGAI/zenpuzzle_partially_observable.yaml'

    yaml_path = '../resources/rataban.yaml'


    # yaml_path = 'zelda.yaml'
    level = 0

    wrapper.build_gym_from_yaml(environment_name, yaml_path,
                                player_observer_type=gd.ObserverType.BLOCK_2D,
                                global_observer_type=gd.ObserverType.BLOCK_2D, level=level)
    env = gym.make(f'GDY-{environment_name}-v0')
    env.enable_history(True)
    env.reset()
    play(env, callback=callback(env), fps=10, zoom=3)
