import gym
import griddly
import numpy as np

env = gym.make('GDY-Partially-Observable-Clusters-v0', shader_path='shaders/compiled')
env.reset()


for a in range(10000):
    obs, reward, done, info = env.step(env.action_space.sample())

    #env.render()
    env.render(observer='global')

    if done:
        env.reset()