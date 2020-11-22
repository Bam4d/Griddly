from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import VideoRecorder
from griddly.util.rts_tools import InvalidMaskingRTSWrapper

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml("GriddlyRTS-Adv",
                                'RTS/GriddlyRTS.yaml',
                                global_observer_type=gd.ObserverType.ISOMETRIC,
                                player_observer_type=gd.ObserverType.ISOMETRIC,
                                level=1)

    env = InvalidMaskingRTSWrapper(gym.make(f'GDY-GriddlyRTS-Adv-v0'))

    env.reset()

    start = timer()

    frames = 0

    fps_samples = []

    observation_shape = env.observation_space.shape

    player1_recorder = VideoRecorder()
    player1_recorder.start("player1_video_test.mp4", observation_shape)

    player2_recorder = VideoRecorder()
    player2_recorder.start("player2_video_test.mp4", observation_shape)

    global_recorder = VideoRecorder()
    global_recorder.start("global_video_test.mp4", observation_shape)

    for s in range(1000):

        frames += 1

        action = env.action_space.sample()

        player_id = action[0]

        obs, reward, done, info = env.step(action)
        global_obs = env.render(observer='global', mode='rgb_array')

        if player_id == 0:
            player1_recorder.add_frame(obs)
        else:
            player2_recorder.add_frame(obs)

        global_recorder.add_frame(global_obs.swapaxes(0, 2))

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()

    # # Have to close the video recorders
    # player1_recorder.close()
    # player2_recorder.close()
    # global_recorder.close()
    print(f'mean fps: {np.mean(fps_samples)}')
