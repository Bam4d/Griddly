import torch
from torch.distributions import Categorical


class TorchConditionalMaskingGridnetExploration():

    def __init__(self, model, dist_class, dist_inputs, valid_action_trees):
        self._valid_action_trees = valid_action_trees
        self._dist_class = dist_class

        self._num_inputs = dist_inputs.shape[0]

        self._width = model.width
        self._height = model.height
        self._grid_action_shape = model.grid_action_shape
        self._grid_action_parts = len(self._grid_action_shape)
        self._grid_channels = model.grid_channels

        self._dist_inputs_reshaped = dist_inputs.reshape(-1, self._grid_channels, self._width, self._height)

    def _mask_and_sample(self, options, logits):

        mask = torch.zeros([logits.shape[0]])
        mask[options] = 1

        logits += torch.log(mask)
        dist = Categorical(logits=logits)
        sampled = dist.sample()
        logp = dist.log_prob(sampled)

        return sampled, logits, logp, mask

    def get_actions_and_mask(self):

        actions = torch.zeros([self._num_inputs, self._grid_action_parts, self._width, self._height])
        masked_logits = torch.zeros([self._num_inputs, self._grid_channels, self._width, self._height])
        mask = torch.zeros([self._num_inputs, self._grid_channels, self._width, self._height])
        logp_sums = torch.zeros([self._num_inputs])

        # Initialize the masks to NOP
        mask[:, 0, :, :] = 1
        for action_logit_size in self._grid_action_shape[:-1]:
            mask[:, action_logit_size, :, :] = 0

        for i in range(self._num_inputs):
            if len(self._valid_action_trees) >= 1:

                x_tree = self._valid_action_trees[i]

                # In the case there are no available actions for the player
                if len(x_tree) == 0:
                    continue

                # only bother with calculating actions for things that are possible in the grid

                for x, y_tree in x_tree.items():
                    for y, subtree in y_tree.items():

                        subtree_options = list(subtree.keys())

                        dist_input = self._dist_inputs_reshaped[i, :, x, y]
                        dist_input_split = torch.split(dist_input, tuple(self._grid_action_shape), dim=0)

                        logp_parts = torch.zeros([self._grid_action_parts])
                        mask_offset = 0
                        for a in range(self._grid_action_parts):
                            dist_part = dist_input_split[a]

                            sampled, masked_part_logits, logp, mask_part = self._mask_and_sample(
                                subtree_options,
                                dist_part
                            )

                            # Set the action and the mask for each part of the action
                            logit_end = mask_offset + self._grid_action_shape[a]
                            actions[i, a] = sampled
                            masked_logits[i, mask_offset:logit_end, x, y] = masked_part_logits
                            mask[i, mask_offset:logit_end, x, y] = mask_part

                            logp_parts[a] = logp

                            if mask_part.sum() == 0:
                                raise RuntimeError('mask calculated incorrectly')

                            mask_offset += self._grid_action_shape[a]

                            if isinstance(subtree, dict):
                                subtree = subtree[int(sampled)]
                                if isinstance(subtree, dict):
                                    subtree_options = list(subtree.keys())
                                else:
                                    # Leaf nodes with action_id list
                                    subtree_options = subtree

                        logp_sums[i] += torch.sum(logp_parts)

        return actions.flatten(1), masked_logits.flatten(1), logp_sums, mask.flatten(1)
