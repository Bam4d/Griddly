from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd

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

    for s in range(10000):

        frames += 1

        player = np.random.randint(player_count)
        defined_action = np.random.randint(defined_actions_count)

        action = [player, defined_action]
        action.extend(env.action_space.sample())

        obs, reward, done, info = env.step(action)
        env.render()

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()
    print(f'mean fps: {np.mean(fps_samples)}')
