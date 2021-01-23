from stable_baselines3.common.vec_env import SubprocVecEnv
import numpy as np
import gym
import griddly

game = "GDY-Partially-Observable-Zelda-v0"

def make_env():
    def _monad():
        env = gym.make(game)
        return env
    return _monad

if __name__ == '__main__':
    raw_list = [make_env() for _ in range(10)]
    envs = SubprocVecEnv(raw_list)

    init_obs = envs.reset()

    for i in range(10000):

        envs.step(np.zeros((10,2)))
        envs.render()