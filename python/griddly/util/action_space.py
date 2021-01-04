import gym
import numpy as np

class MultiAgentActionSpace(list):
    def __init__(self, agents_action_space):
        for x in agents_action_space:
            assert isinstance(x, gym.spaces.space.Space)

        super(MultiAgentActionSpace, self).__init__(agents_action_space)
        self.agents_action_space = agents_action_space

    def sample(self):
        """ samples action for each agent from uniform distribution"""
        return [agent_action_space.sample() for agent_action_space in self.agents_action_space]

class ValidatedMultiAgentActionSpace(MultiAgentActionSpace):
    """
    The same action space as MultiAgentActionSpace, however sampling this action space only results in valid actions
    """

    def __init__(self, agents_action_space, masking_wrapper):
        self._masking_wrapper = masking_wrapper
        super().__init__(agents_action_space)

    def sample(self, player_id=None):

        sampled_actions = []
        for player_id in range(self.__len__()):

            # Sample a location with valid actions
            available_actions = [a for a in self._masking_wrapper.env.game.get_available_actions(player_id + 1).items()]
            num_available = len(available_actions)
            if num_available == 0:
                return [0, 0, 0, 0]
            else:
                available_actions_choice = np.random.choice(num_available)

            location, actions = available_actions[available_actions_choice]

            available_action_ids = [aid for aid in self._masking_wrapper.env.game.get_available_action_ids(location, list(
                actions)).items() if len(aid[1])>0]

            num_action_ids = len(available_action_ids)

            # If there are no available actions at all, we do a NOP (which is any action_name with action_id 0)
            if num_action_ids == 0:
                action_name_idx = 0
                action_id = 0
            else:
                available_action_ids_choice = np.random.choice(num_action_ids)
                action_name, action_ids = available_action_ids[available_action_ids_choice]
                action_name_idx = self._masking_wrapper.action_names.index(action_name)
                action_id = np.random.choice(action_ids)

            sampled_actions.append([location[0], location[1], action_name_idx, action_id])

        return sampled_actions