from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import VideoRecorder

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    name = '4rooms'

    wrapper.build_gym_from_yaml(name,
                                'Single-Player/Mini-Grid/minigrid-spiders.yaml',
                                player_observer_type=gd.ObserverType.SPRITE_2D,
                                global_observer_type=gd.ObserverType.ISOMETRIC,
                                level=4)

    env = gym.make(f'GDY-{name}-v0')

    env.reset()

    start = timer()

    frames = 0

    fps_samples = []

    player_observation_shape = env.unwrapped.player_observation_shape
    global_observation_shape = env.unwrapped.global_observation_shape

    player_recorder = VideoRecorder()
    player_recorder.start("player_video_test.mp4", player_observation_shape)

    global_recorder = VideoRecorder()
    global_recorder.start("global_video_test.mp4", global_observation_shape)

    for s in range(1000):

        frames += 1
        obs, reward, done, info = env.step(env.action_space.sample())

        env.render()

        #player_observation = env.render(mode='rgb_array')
        global_observation = env.render(observer='global', mode='rgb_array')

        #player_recorder.add_frame(player_observation.swapaxes(0, 2))
        global_recorder.add_frame(global_observation.swapaxes(0, 2))

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()
    print(f'mean fps: {np.mean(fps_samples)}')
