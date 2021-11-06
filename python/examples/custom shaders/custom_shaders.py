import gym
import griddly
import numpy as np

env = gym.make('GDY-Sokoban-v0', shader_path='shaders/compiled')
env.reset()


for a in range(10000):
    obs, reward, done, info = env.step(env.action_space.sample())

    env.render()

    if done:
        env.reset()