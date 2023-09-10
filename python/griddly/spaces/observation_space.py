from typing import List, Union, Optional, Any, Dict

import numpy as np
import numpy.typing as npt
from gymnasium.spaces import Box, Space
from griddly.typing import Observation, ObservationSpace


class MultiAgentObservationSpace(Space[List[Observation]], list):
    def __init__(
        self,
        agents_observation_space: List[ObservationSpace],
        seed: Optional[Union[int, np.random.Generator]] = None,
    ) -> None:
        # None for shape and dtype
        self.agents_observation_space = agents_observation_space
        Space.__init__(self, seed=seed)
        list.__init__(self, agents_observation_space)

    def sample(self, mask: Optional[Any] = None) -> List[Observation]:
        """samples observations for each agent from uniform distribution"""
        return [
            agent_observation_space.sample()
            for agent_observation_space in self.agents_observation_space
        ]

    def contains(self, obs: List[Observation]) -> bool:
        """contains observation"""
        for space, ob in zip(self.agents_observation_space, obs):
            if not space.contains(ob):
                return False
        else:
            return True

    def seed(self, seed: Optional[int] = None) -> list:
        res = []
        for space in self.agents_observation_space:
            res.extend(space.seed(seed))
        return res


class EntityObservationSpace(ObservationSpace):
    def __init__(
        self,
        entity_features: Dict[str, npt.NDArray],
        seed: Optional[Union[int, np.random.Generator]] = None,
    ) -> None:
        self.gym_space = {}
        self.features = entity_features
        for entity_name, features in entity_features.items():
            self.gym_space[entity_name] = Box(low=-100.0, high=100.0, shape=(len(features),))

        super().__init__(seed=seed)

    def contains(self, x: Observation) -> bool:
        """contains observation"""
        for entity_name, features in self.features.items():
            for entity_obs in x["Entities"][entity_name]:
                if len(features) != len(entity_obs):
                    return False
        else:
            return True
