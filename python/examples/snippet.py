import gym
import griddly
from griddly import gd
from griddly.util.wrappers import InvalidMaskingRTSWrapper

if __name__ == '__main__':

    env = gym.make('GDY-GriddlyRTS-v0', level=1, global_observer_type=gd.ObserverType.ISOMETRIC)
    env.reset()
    env = InvalidMaskingRTSWrapper(env)

    # Replace with your own control algorithm!
    for s in range(1000):
        obs, reward, done, info = env.step(env.action_space.sample())
        for p in range(env.player_count):
            env.render(observer=p) # Renders the environment from the perspective of a single player

        env.render(observer='global') # Renders the entire environment