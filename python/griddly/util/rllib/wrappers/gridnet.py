from gym.spaces import MultiDiscrete
import numpy as np
from griddly.util.rllib.wrappers.core import RLlibEnv


class RLlibGridnetEnv(RLlibEnv):

    def __init__(self, env_config):
        super().__init__(env_config)

    def set_transform(self):
        """
        :return:
        """

        super().set_transform()

        num_grid_locations = self.width * self.height
        cell_discrete_action_shape = self.action_space.nvec[2:]

        self.num_action_parts = 1
        if self.action_count > 1:
            self.num_action_parts += 1

        self.num_action_logits = np.sum(cell_discrete_action_shape)

        cell_multi_discretes = []
        for g in range(num_grid_locations):
            cell_multi_discretes.extend(cell_discrete_action_shape)

        self.action_space = MultiDiscrete(cell_multi_discretes)

    def step(self, action):
        # Un-grid the actions

        grid_actions = action.reshape(-1, self.num_action_parts, self.width, self.height)

        # We have a HxW grid of actions, but we only need the actions for the valid locations in the grid,
        # so we pull them out
        multi_actions = []
        for p in range(self.player_count):
            unit_actions = []
            x_tree = self.last_valid_action_trees[p]
            for x, y_tree in x_tree.items():
                for y, _ in y_tree.items():
                    unit_action = grid_actions[p, :, x, y]
                    unit_actions.append([x, y, *unit_action])
            multi_actions.append(unit_actions)

        return super().step(multi_actions)
