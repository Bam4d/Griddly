import gym
import numpy as np
from griddly.RenderTools import RenderToFile

if __name__ == '__main__':

    env = gym.make('GDY-Clusters-v0')
    env.reset()

    file_render = RenderToFile()
    # Replace with your own control algorithm!
    for s in range(5):
        obs, reward, done, info = env.step(env.action_space.sample())
        # env.render()
        #
        # env.render(observer='global')

        file_render.render(obs, f'clusters-{s}.png')