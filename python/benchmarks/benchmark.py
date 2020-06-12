from timeit import default_timer as timer
import numpy as np
import gym
import os
import psutil
process = psutil.Process(os.getpid())

def griddly(gdy_file):
    from griddly import GymWrapperFactory, gd

    wrapper = GymWrapperFactory()
    wrapper.build_gym_from_yaml("Griddly", gdy_file, player_observer_type=gd.ObserverType.SPRITE_2D,
                                level=0)
    return gym.make(f'GDY-Griddly-v0')

def micro_rts_test():
    import gym_microrts
    return gym.make('BWDistantResources32x32-v0')

def minigrid_test():
    import gym_minigrid
    return gym.make("MiniGrid-FourRooms-v0")

def gvgai_test():
    import gvgai
    return gym.make("gvgai-sokoban-lvl0-v0")

def gvgai_test_old():
    import gym_gvgai
    return gym_gvgai.make("gvgai-sokoban-lvl0-v0")

if __name__ == '__main__':

    #env = minigrid_test()
    #env = griddly('single-player/minigrid-4rooms.yaml')

    env = micro_rts_test()
    #env = griddly('RTS/BWDistantResources32x32.yaml')

    # memory usage recorded in these tests is inaccurate because the GVGAI environment is in a different process
    #env = gvgai_test()
    #env = gvgai_test_old()
    #env = griddly('single-player/sokoban.yaml')

    env.reset()
    start = timer()

    frames = 0

    fps_samples = []
    mem_samples = []

    for s in range(10000):

        frames += 1
        obs, reward, done, info = env.step(env.action_space.sample())

        rendered_obs = env.render(mode='rgb_array')

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            mem = process.memory_info().rss
            mem_samples.append(mem)
            fps_samples.append(fps)
            print(f'fps: {fps}')
            print(f'mem: {mem}')
            frames = 0
            start = timer()
    print(f'mean fps: {np.mean(fps_samples)}')
    print(f'mean mem: {np.mean(mem_samples)}')