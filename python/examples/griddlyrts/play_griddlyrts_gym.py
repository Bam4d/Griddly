from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import VideoRecorder
from griddly.util.wrappers import InvalidMaskingRTSWrapper

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml("GriddlyRTS-Adv",
                                'RTS/Stratega/kill-the-king.yaml',
                                global_observer_type=gd.ObserverType.SPRITE_2D,
                                player_observer_type=gd.ObserverType.ISOMETRIC,
                                level=0)

    env_original = gym.make(f'GDY-GriddlyRTS-Adv-v0')
    # env_original = gym.make(f'GDY-GriddlyRTS-Adv-v0')

    env_original.reset()

    env = InvalidMaskingRTSWrapper(env_original)

    start = timer()

    frames = 0

    fps_samples = []

    player1_recorder = VideoRecorder()
    player1_visualization = env.render(observer=0, mode='rgb_array')
    player1_recorder.start("player1_video_test.mp4", player1_visualization.shape)

    player2_recorder = VideoRecorder()
    player2_visualization = env.render(observer=1, mode='rgb_array')
    player2_recorder.start("player2_video_test.mp4", player2_visualization.shape)

    global_recorder = VideoRecorder()
    global_visualization = env.render(observer='global', mode='rgb_array')
    global_recorder.start("global_video_test.mp4", global_visualization.shape)

    for s in range(10000):

        frames += 1

        action = env.action_space.sample()

        # player_1_mask = env.get_unit_location_mask(0, mask_type='reduced')
        # player_2_mask = env.get_unit_location_mask(1)
        #
        # action_masks = env.get_unit_action_mask([6, 3], ['gather', 'move'], padded=False)

        env.render(observer='global')
        env.render(observer=0)
        env.render(observer=1)

        obs, reward, done, info = env.step(action)

        global_observation = env.render(mode='rgb_array', observer='global')

        global_recorder.add_frame(global_observation)
        player1_recorder.add_frame(obs[0].swapaxes(0, 2))
        player2_recorder.add_frame(obs[1].swapaxes(0, 2))

        if done:
            state = env.get_state()
            print(state)
            print(info)

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
