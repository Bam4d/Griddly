import gym
import numpy as np
from griddly.util.vector_visualization import Vector2RGB

from griddly import GymWrapper


class ValidatedMultiDiscrete(gym.spaces.MultiDiscrete):
    """
    The same action space as MultiDiscrete, however sampling this action space only results in valid actions
    """

    def __init__(self, nvec, masking_wrapper):
        self._masking_wrapper = masking_wrapper
        super().__init__(nvec)

    def sample(self, player_id=None):
        if player_id is None:
            player_id = np.random.choice(self._masking_wrapper.player_count)

        # Sample a location with valid actions
        available_actions = [a for a in self._masking_wrapper.env.game.get_available_actions(player_id + 1).items()]
        available_actions_choice = np.random.choice(len(available_actions))
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

        return [player_id, location[0], location[1], action_name_idx, action_id]


class InvalidMaskingRTSWrapper(gym.Wrapper):
    """
    Creates a wrapper that can produce invalid action masking.
    
    the action space returned is a special case of MultiDiscrete called ValidatedMultiDiscrete.
    The ValidatedMultiDiscrete actions space only produces actions that are valid in the game when sampling.
    
    The action space shape is in the following format:
    [player_id, grid_height, grid_width, action_name_id, action_id]
    
    The action_name_id is an index of the action name in self.action_names object.
    The action_id is the action_id of an action.

    Invalid action masks can be generated using get_unit_location_mask and get_unit_action_mask.
    Please refer to https://arxiv.org/abs/2006.14171 to understand more about invalid action masking to
    policy gradient methods.
    """

    def __init__(self, env):
        super().__init__(env)

    def step(self, action):
        player_id = action[0]
        x = action[1]
        y = action[2]
        action_name = self.action_names[action[3]]
        action_id = action[4]

        action_data = [x, y, action_id]

        reward, done, info = self.env._players[player_id].step(action_name, action_data)
        self.env._player_last_observation[player_id] = np.array(self.env._players[player_id].observe(), copy=False)
        return self.env._player_last_observation[player_id], reward, done, info

    def reset(self, level_id=None, level_string=None):

        reset_result = super().reset(level_id=level_id, level_string=level_string)

        # Overwrite the action space
        self.env.action_space = self._create_action_space()
        self.action_space = self.env.action_space
        self.observation_space = self.env.observation_space

        return reset_result

    def get_unit_location_mask(self, player_id, mask_type='full'):
        """
        Returns a mask for grid_height and grid_width giving the available action locations.
        :param player_id: The player to generate masks for
        :param mask_type: If 'full' a mask the size of grid_width*grid_height is returned.
        If 'reduced' a separate mask for grid_height and grid_width is returned. This mask is significantly smaller,
        but allows the agent to still choose invalid actions.
        """

        if mask_type == 'full':

            grid_mask = np.zeros((self._grid_width, self._grid_height))
            for location, action_names in self.env.game.get_available_actions(player_id + 1).items():
                grid_mask[[location[0]], [location[1]]] = 1
            return grid_mask

        elif mask_type == 'reduced':

            grid_width_mask = np.zeros(self._grid_width)
            grid_height_mask = np.zeros(self._grid_height)
            for location, action_names in self.env.game.get_available_actions(player_id + 1).items():
                grid_width_mask[location[0]] = 1
                grid_height_mask[location[1]] = 1
            return grid_height_mask, grid_width_mask

    def get_unit_action_mask(self, location, action_names, padded=True):
        """
        Given a location and a list of action names, return a mask the valid actions for each action_name

        :param location: the location of the unit to get action masks
        :param action_names: a list of action names to
        :param padded: If set to true, the masks will always be padded to the length of self.max_action_ids
        :return:
        """
        action_masks = {}
        for action_name, action_ids in self.env.game.get_available_action_ids(location, action_names).items():
            mask_size = self.max_action_ids if padded else self.valid_action_mappings[action_name]
            action_ids_mask = np.zeros(mask_size)
            # action_id 0 is always a NOP
            action_ids_mask[0] = 1
            action_ids_mask[action_ids] = 1
            action_masks[action_name] = action_ids_mask

        return action_masks

    def _create_action_space(self):

        # Convert action to GriddlyActionASpace
        self.player_count = self.env._grid.get_player_count()
        self.action_input_mappings = self.env._grid.get_action_input_mappings()

        self._grid_width = self.env._grid.get_width()
        self._grid_height = self.env._grid.get_height()

        self.avatar_object = self.env._grid.get_avatar_object()

        has_avatar = self.avatar_object is not None and len(self.avatar_object) > 0

        if has_avatar:
            raise RuntimeError("Cannot use MultiDiscreteRTSWrapper with environments that control single avatars")

        self.valid_action_mappings = {}
        self.action_names = []
        self.max_action_ids = 0
        for action_name, mapping in sorted(self.action_input_mappings.items()):
            if not mapping['Internal']:
                self.action_names.append(action_name)
                num_action_ids = len(mapping['InputMappings'])+1
                if self.max_action_ids < num_action_ids:
                    self.max_action_ids = num_action_ids
                self.valid_action_mappings[action_name] = num_action_ids

        multi_discrete_space = [self.player_count, self._grid_width, self._grid_height, len(self.valid_action_mappings),
                                self.max_action_ids]
        return ValidatedMultiDiscrete(multi_discrete_space, self)
