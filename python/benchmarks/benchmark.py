#!/home/bam4d/anaconda3/envs/griddly/bin/python3
from timeit import default_timer as timer
import numpy as np
import gym
import os
import psutil
from gym import register

# from griddly.util.wrappers import InvalidMaskingRTSWrapper

process = psutil.Process(os.getpid())


def griddly(gdy_file):
    from griddly import GymWrapperFactory, gd

    wrapper = GymWrapperFactory()
    wrapper.build_gym_from_yaml(
        "Griddly", gdy_file, player_observer_type=gd.ObserverType.SPRITE_2D, level=0
    )
    return gym.make(f"GDY-Griddly-v0")


def micro_rts_test():
    import gym_microrts

    return gym.make("MicrortsMining-v4")


def minigrid_test():
    import gym_minigrid

    return gym.make("MiniGrid-FourRooms-v0")


def gvgai_test():
    import gvgai

    return gym.make("gvgai-sokoban-lvl0-v0")


def gvgai_test_old():
    import gym_gvgai

    return gym_gvgai.make("gvgai-sokoban-lvl0-v0")


if __name__ == "__main__":

    # env = griddly('dmlab_pushbox.yaml')

    # env = minigrid_test()
    env = griddly("Single-Player/Mini-Grid/minigrid-eyeball.yaml")

    # env = micro_rts_test()
    # env = griddly('MicrortsMining.yaml')

    # memory usage recorded in these tests is inaccurate because the GVGAI environment is in a different process
    # env = gvgai_test()
    # env = gvgai_test_old()
    # env = griddly('Single-Player/GVGAI/sokoban.yaml')

    env.reset()
    # env = InvalidMaskingRTSWrapper(env)
    start = timer()

    frames = 0

    fps_samples = []
    mem_samples = []

    for s in range(100000):

        frames += 1
        obs, reward, done, info = env.step(env.action_space.sample())

        # env.render()
        # rendered_obs = env.render(mode='rgb_array')

        # state = env.get_state()

        if frames % 1000 == 0:
            end = timer()
            fps = frames / (end - start)
            mem = process.memory_info().rss
            mem_samples.append(mem)
            fps_samples.append(fps)
            print(f"fps: {fps}")
            print(f"mem: {mem}")
            frames = 0
            start = timer()

        if done:
            env.reset()
    print(f"mean fps: {np.mean(fps_samples)}, std: {np.std(fps_samples)}")
    print(f"mean mem: {np.mean(mem_samples)}")
