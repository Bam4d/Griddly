from typing import Optional

import gym
from gym.spaces.space import T_cov


class MultiAgentObservationSpace(list):
    def __init__(self, agents_observation_space):
        for x in agents_observation_space:
            assert isinstance(x, gym.spaces.space.Space)

        super().__init__(agents_observation_space)
        self._agents_observation_space = agents_observation_space

    def sample(self):
        """samples observations for each agent from uniform distribution"""
        return [
            agent_observation_space.sample()
            for agent_observation_space in self._agents_observation_space
        ]

    def contains(self, obs):
        """contains observation"""
        for space, ob in zip(self._agents_observation_space, obs):
            if not space.contains(ob):
                return False
        else:
            return True

    def seed(self, seed):
        for space in self._agents_observation_space:
            space.seed(seed)

class EntityObservationSpace(gym.spaces.Dict):

    def __init__(self, entity_features, seed=None):
        gym_space = {}
        self.features = entity_features
        for entity_name, features in entity_features.items():
            gym_space[entity_name] = gym.spaces.Box(low=-100.0, high=100.0, shape=(len(features),))

        super().__init__(gym_space, seed)