from griddly import GymWrapperFactory, gd, GymWrapper
import timeit

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    env = GymWrapper('Single-Player/GVGAI/sokoban.yaml',
                     player_observer_type=gd.ObserverType.BLOCK_2D,
                     global_observer_type=gd.ObserverType.SPRITE_2D,
                     level=0, max_steps=500)

    env.reset()

    start = timeit.default_timer()

    frames = 0

    for i in range(200000):

        obs, reward, done, info = env.step(env.action_space.sample())

        frames += 1

        #env.render()

        if done:

            end = timeit.default_timer()
            print(f'{frames/(end - start)} SPS')
            frames = 0
            env.reset()
            start = timeit.default_timer()