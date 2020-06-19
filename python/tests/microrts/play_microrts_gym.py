from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import VideoRecorder

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml("BWDistantResources32x32", 'RTS/BWDistantResources32x32.yaml', player_observer_type=gd.ObserverType.SPRITE_2D, level=0)

    env = gym.make(f'GDY-BWDistantResources32x32-v0')

    env.reset()

    start = timer()

    frames = 0

    fps_samples = []

    player_count = env.unwrapped.player_count
    defined_actions_count = env.unwrapped.defined_actions_count

    observation_shape = env.observation_space.shape

    player_recorder = VideoRecorder()
    player_recorder.start("player_video_test.mp4", observation_shape)

    global_recorder = VideoRecorder()
    global_recorder.start("global_video_test.mp4", observation_shape)

    for s in range(100):

        frames += 1

        player = np.random.randint(player_count)
        defined_action = np.random.randint(defined_actions_count)

        action = [player, defined_action]
        action.extend(env.action_space.sample())

        obs, reward, done, info = env.step(action)
        global_obs = env.render(observer='global', mode='rgb_array')

        player_recorder.add_frame(obs)
        global_recorder.add_frame(global_obs.swapaxes(0, 2))

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()
    print(f'mean fps: {np.mean(fps_samples)}')
