from timeit import default_timer as timer

import gym

from griddly_python import GymWrapperFactory

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml("BWDistantResources32x32", 'RTS/BWDistantResources32x32.yaml', level=0)

    env = gym.make(f'GDY-BWDistantResources32x32-v0')

    env.reset()

    start = timer()

    frames = 0

    fps_samples = []

    for s in range(10000):

        frames += 1
        env.step(env.action_space.sample())
        env.render(mode='rgb_array')

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()
