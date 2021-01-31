import gym

from griddly import GymWrapper


class RLibWrapper(GymWrapper):

    def __init__(self, env_config):
        super().__init__(**env_config)

        print(env.observation_space.shape)
        env.observation_space = gym.spaces.Box(
            env.observation_space.low.transpose((1, 2, 0)),
            env.observation_space.high.transpose((1, 2, 0))
        )
        env = TransformObservation(env, lambda obs: obs.transpose(1, 2, 0))
        print(env.observation_space.shape)

        reset()

    def reset(self, **kwargs):
        observation = self.env.reset(**kwargs)
        return self.transform(observation)

    def step(self, action):
        observation, reward, done, info = self.env.step(action)
        return self.transform(observation), reward, done, info

    def observation(self, observation):
        raise NotImplementedError