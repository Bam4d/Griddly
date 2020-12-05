import gym
from griddly import gd

if __name__ == '__main__':

    env = gym.make('GDY-Spider-Nest-v0', global_observer_type=gd.ObserverType.ISOMETRIC)
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):
        obs, reward, done, info = env.step(env.action_space.sample())
        #env.render()

        #env.render(observer='global')
