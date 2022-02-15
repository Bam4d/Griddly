import gym
import numpy as np

from griddly.util.action_space import ValidatedActionSpace


class ValidActionSpaceWrapper(gym.Wrapper):
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

        if env.action_space is None or env.observation_space is None:
            raise RuntimeError(
                "Please reset the environment before applying the InvalidMaskingRTSWrapper"
            )

        super().__init__(env)
        self.action_space = self._override_action_space()

    def get_unit_location_mask(self, player_id, mask_type="full"):
        """
        Returns a mask for grid_height and grid_width giving the available action locations.
        :param player_id: The player to generate masks for
        :param mask_type: If 'full' a mask the size of grid_width*grid_height is returned.
        If 'reduced' a separate mask for grid_height and grid_width is returned. This mask is significantly smaller,
        but allows the agent to still choose invalid actions.
        """

        assert player_id <= self.player_count, "Player does not exist."
        assert player_id > 0, "Player 0 is reserved for internal actions only."

        if mask_type == "full":

            grid_mask = np.zeros((self.grid_width, self.grid_height))
            for location, action_names in self.env.game.get_available_actions(
                player_id
            ).items():
                grid_mask[[location[0]], [location[1]]] = 1
            return grid_mask

        elif mask_type == "reduced":

            grid_width_mask = np.zeros(self._grid_width)
            grid_height_mask = np.zeros(self._grid_height)
            for location, action_names in self.env.game.get_available_actions(
                player_id
            ).items():
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
        for action_name, action_ids in self.env.game.get_available_action_ids(
            location, action_names
        ).items():
            mask_size = (
                self.max_action_ids if padded else self.num_action_ids[action_name]
            )
            action_ids_mask = np.zeros(mask_size)
            # action_id 0 is always a NOP
            action_ids_mask[0] = 1
            action_ids_mask[action_ids] = 1
            action_masks[action_name] = action_ids_mask

        return action_masks

    def _override_action_space(self):
        return ValidatedActionSpace(self.action_space, self)

    def clone(self):
        cloned_env = ValidActionSpaceWrapper(self.env.clone())
        return cloned_env
