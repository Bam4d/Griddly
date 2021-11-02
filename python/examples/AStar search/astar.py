import os

from griddly import GymWrapperFactory, gd, GymWrapper
from griddly.RenderTools import VideoRecorder

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    name = 'astar_opponent_environment'

    current_path = os.path.dirname(os.path.realpath(__file__))

    env = GymWrapper('astar_opponent_environment.yaml',
                     player_observer_type=gd.ObserverType.VECTOR,
                     global_observer_type=gd.ObserverType.SPRITE_2D,
                     level=0)

    env.reset()

    global_recorder = VideoRecorder()
    global_visualization = env.render(observer='global', mode='rgb_array')
    global_recorder.start("global_video_test.mp4", global_visualization.shape)

    for i in range(1000):

        obs, reward, done, info = env.step(env.action_space.sample())

        env.render(observer='global')
        frame = env.render(observer='global', mode='rgb_array')

        global_recorder.add_frame(frame)

        if done:
            env.reset()

    global_recorder.close()