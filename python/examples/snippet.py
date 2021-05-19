from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd
from griddly.util.wrappers import ValidActionSpaceWrapper


def make_env(name):
    wrapper = GymWrapperFactory()
    wrapper.build_gym_from_yaml(name, 'Single-Player/Mini-Grid/minigrid-spiders.yaml',
                                player_observer_type=gd.ObserverType.SPRITE_2D,
                                global_observer_type=gd.ObserverType.BLOCK_2D,
                                level=0,
                                max_steps=200)

    env = gym.make(f'GDY-{name}-v0')
    env.enable_history(True)
    env.reset()

    return ValidActionSpaceWrapper(env)


if __name__ == '__main__':

    env = make_env("Test")

    start = timer()

    frames = 0

    fps_samples = []


    for s in range(100000):

        action = env.action_space.sample()

        frames += 1
        obs, reward, done, info = env.step(action)
        #env.render()
        env.render(observer='global')

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()

        if done:
            env.reset()
    print(f'mean fps: {np.mean(fps_samples)}')
