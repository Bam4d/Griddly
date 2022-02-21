import gym
import numpy as np


class MultiAgentActionSpace(list):
    def __init__(self, agents_action_space):
        for x in agents_action_space:
            assert isinstance(x, gym.spaces.space.Space)

        super(MultiAgentActionSpace, self).__init__(agents_action_space)
        self.agents_action_space = agents_action_space

    def sample(self):
        """samples action for each agent from uniform distribution"""
        return [
            agent_action_space.sample()
            for agent_action_space in self.agents_action_space
        ]

    def seed(self, seed):
        for space in self.agents_action_space:
            space.seed(seed)



class ValidatedActionSpace(gym.spaces.space.Space, list):
    """
    Sampling this action space only results in valid actions
    """

    def __init__(self, action_space, masking_wrapper):
        self._masking_wrapper = masking_wrapper

        shape = None
        dtype = None

        if isinstance(action_space, gym.spaces.Discrete) or isinstance(
            action_space, gym.spaces.MultiDiscrete
        ):
            shape = action_space.shape
            dtype = action_space.dtype
        elif isinstance(action_space, MultiAgentActionSpace):
            shape = action_space[0].shape
            dtype = action_space[0].dtype

        self.action_space = action_space

        super().__init__(shape, dtype)

    def __len__(self):
        if isinstance(self.action_space, list):
            return len(self.action_space)
        else:
            return 1

    def __getitem__(self, y):
        if isinstance(self.action_space, list):
            return self.action_space[y]
        else:
            raise IndexError()

    def __getattr__(self, name):
        if name.startswith("_"):
            raise AttributeError(
                "attempted to get missing private attribute '{}'".format(name)
            )
        return getattr(self.action_space, name)

    def _sample_valid(self, player_id):
        # Sample a location with valid actions

        assert player_id <= self._masking_wrapper.player_count, "Player does not exist."
        assert player_id > 0, "Player 0 is reserved for internal actions only."

        available_actions = [
            a
            for a in self._masking_wrapper.game.get_available_actions(player_id).items()
        ]
        num_available = len(available_actions)
        if num_available == 0:
            return [0, 0, 0, 0]
        else:
            available_actions_choice = self.np_random.choice(num_available)

        location, actions = available_actions[available_actions_choice]

        available_action_ids = [
            aid
            for aid in self._masking_wrapper.game.get_available_action_ids(
                location, list(actions)
            ).items()
            if len(aid[1]) > 0
        ]

        num_action_ids = len(available_action_ids)

        # If there are no available actions at all, we do a NOP (which is any action_name with action_id 0)
        if num_action_ids == 0:
            action_name_idx = 0
            action_id = 0
        else:
            available_action_ids_choice = self.np_random.choice(num_action_ids)
            action_name, action_ids = available_action_ids[available_action_ids_choice]
            action_name_idx = self._masking_wrapper.action_names.index(action_name)
            action_id = self.np_random.choice(action_ids)

        sampled_action = []

        # Build the action based on the action_space info
        if not self._masking_wrapper.has_avatar:
            sampled_action.extend([location[0], location[1]])

        if self._masking_wrapper.action_count > 1:
            sampled_action.append(action_name_idx)

        sampled_action.append(action_id)

        return sampled_action

    def sample(self, player_id=None):

        if player_id is not None:
            return self._sample_valid(player_id)

        if self._masking_wrapper.player_count == 1:
            return self._sample_valid(1)

        sampled_actions = []
        for player_id in range(self._masking_wrapper.player_count):
            sampled_actions.append(self._sample_valid(player_id + 1))

        return sampled_actions
