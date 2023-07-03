from gymnasium.spaces import Space, Box, Dict


class MultiAgentObservationSpace(Space, list):
    def __init__(self, agents_observation_space, seed=None):
        for x in agents_observation_space:
            assert isinstance(x, Space)

        # None for shape and dtype
        self.agents_observation_space = agents_observation_space
        Space.__init__(self, None, None, seed) 
        list.__init__(self, agents_observation_space)

    def sample(self):
        """samples observations for each agent from uniform distribution"""
        return [
            agent_observation_space.sample()
            for agent_observation_space in self.agents_observation_space
        ]

    def contains(self, obs):
        """contains observation"""
        for space, ob in zip(self.agents_observation_space, obs):
            if not space.contains(ob):
                return False
        else:
            return True

    def seed(self, seed):
        for space in self.agents_observation_space:
            space.seed(seed)


class EntityObservationSpace(Space):
    def __init__(self, entity_features, seed=None):
        gym_space = {}
        self.features = entity_features
        for entity_name, features in entity_features.items():
            gym_space[entity_name] = Box(
                low=-100.0, high=100.0, shape=(len(features),)
            )

        super().__init__(gym_space, seed)
    
    def contains(self, x) -> bool:
        """contains observation"""
        for entity_name, features in self.features.items():
            for entity_obs in x["Entities"][entity_name]:
                if len(features) != len(entity_obs):
                    return False
        else:
            return True
