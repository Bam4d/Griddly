from ray.rllib.models import ActionDistribution
from ray.rllib.models.torch.torch_action_dist import TorchDistributionWrapper
import numpy as np
import torch
from ray.rllib.utils.typing import TensorType
from torch.distributions import Categorical


class GridnetMaskedCategoricalDistribution(TorchDistributionWrapper):

    def __init__(self, dist_inputs, model, valid_action_trees):
        self._valid_action_trees = valid_action_trees

        self._num_inputs = dist_inputs.shape[0]

        self._width = model.width
        self._height = model.height
        self._grid_action_shape = model.grid_action_shape
        self._grid_action_parts = len(self._grid_action_shape)
        self._grid_channels = model.grid_channels
        self._dist_inputs_reshaped = dist_inputs.reshape(-1, self._grid_channels, self._width, self._height)

    def _is_dummy(self):
        # Hack for annoying rllib optimization where it tries to work out waht values you are using in your algorithm
        return not isinstance(self._valid_action_trees[0], dict)

    def _reset(self):
        self._last_sample = torch.zeros([self._num_inputs, self._grid_action_parts, self._width, self._height])
        self._last_sample_masks = torch.zeros([self._num_inputs, self._grid_channels, self._width, self._height])

        self._last_sample_logp = torch.zeros([self._num_inputs])

        # Initialize the masks to NOP
        self._last_sample_masks[:, 0, :, :] = 1
        for action_logit_size in self._grid_action_shape[:-1]:
            self._last_sample_masks[:, action_logit_size, :, :] = 1

        self._last_sample_masked_logits = self._dist_inputs_reshaped + torch.log(self._last_sample_masks)

    def deterministic_sample(self):

        self._reset()

        for i in range(self._num_inputs):
            x_tree = self._valid_action_trees[i]

            # In the case there are no available actions for the player
            if len(x_tree) == 0:
                continue
            for x, y_tree in x_tree.items():
                for y, subtree in y_tree.items():

                    subtree_options = list(subtree.keys())

                    dist_input = self._dist_inputs_reshaped[i, :, x, y]
                    dist_input_split = torch.split(dist_input, tuple(self._grid_action_shape), dim=0)

                    entropy_parts = torch.zeros([self._grid_action_parts])
                    for a in range(self._grid_action_parts):
                        dist_part = dist_input_split[a]
                        mask = torch.zeros([dist_part.shape[0]])
                        mask[subtree_options] = 1

                        logits = dist_part + torch.log(mask)
                        dist = Categorical(logits=logits)
                        sampled = torch.argmax(dist.probs)

                        self._last_sample[i, a, x, y] = sampled

        return self._last_sample

    def logp(self, actions):

        logp_sums = torch.zeros([self._num_inputs])

        if self._is_dummy():
            return logp_sums

        grid_actions = actions.reshape(-1, self._grid_action_parts, self._width, self._height)

        # run through the trees and only calculate where valid actions exist
        for i in range(self._num_inputs):
            x_tree = self._valid_action_trees[i]

            # In the case there are no available actions for the player
            if len(x_tree) == 0:
                continue
            for x, y_tree in x_tree.items():
                for y, subtree in y_tree.items():

                    subtree_options = list(subtree.keys())

                    dist_input = self._dist_inputs_reshaped[i, :, x, y]
                    dist_input_split = torch.split(dist_input, tuple(self._grid_action_shape), dim=0)

                    logp_parts = torch.zeros([self._grid_action_parts])
                    for a in range(self._grid_action_parts):
                        dist_part = dist_input_split[a]
                        mask = torch.zeros([dist_part.shape[0]])
                        mask[subtree_options] = 1

                        logits = dist_part + torch.log(mask)
                        dist = Categorical(logits=logits)
                        logp_parts[a] = dist.log_prob(grid_actions[i, a, x, y])

                    logp_sums[i] += torch.sum(logp_parts)

        return logp_sums

    def entropy(self):

        entropy_sums = torch.zeros([self._num_inputs])

        if self._is_dummy():
            return entropy_sums

        # Entropy for everything by the valid action locations will be 0
        # as the masks only allow selection of a single action
        for i in range(self._num_inputs):
            x_tree = self._valid_action_trees[i]

            # In the case there are no available actions for the player
            if len(x_tree) == 0:
                continue
            for x, y_tree in x_tree.items():
                for y, subtree in y_tree.items():

                    subtree_options = list(subtree.keys())

                    dist_input = self._dist_inputs_reshaped[i, :, x, y]
                    dist_input_split = torch.split(dist_input, tuple(self._grid_action_shape), dim=0)

                    entropy_parts = torch.zeros([self._grid_action_parts])
                    for a in range(self._grid_action_parts):
                        dist_part = dist_input_split[a]
                        mask = torch.zeros([dist_part.shape[0]])
                        mask[subtree_options] = 1

                        logits = dist_part + torch.log(mask)
                        dist = Categorical(logits=logits)
                        entropy_parts[a] = dist.entropy()

                    entropy_sums[i] += torch.sum(entropy_parts)

        return entropy_sums

    def kl(self, other: ActionDistribution):
        pass

    def _mask_and_sample(self, options, logits):

        mask = torch.zeros([logits.shape[0]])
        mask[options] = 1

        logits += torch.log(mask)
        dist = Categorical(logits=logits)
        sampled = dist.sample()
        logp = dist.log_prob(sampled)

        return sampled, logp, logits, mask

    def sample(self):

        self._reset()

        for i in range(self._num_inputs):
            if len(self._valid_action_trees) >= 1:

                x_tree = self._valid_action_trees[i]

                # In the case there are no available actions for the player
                if len(x_tree) == 0:
                    continue

                # Only bother with calculating actions for things that are possible in the grid
                for x, y_tree in x_tree.items():
                    for y, subtree in y_tree.items():

                        subtree_options = list(subtree.keys())

                        dist_input = self._dist_inputs_reshaped[i, :, x, y]
                        dist_input_split = torch.split(dist_input, tuple(self._grid_action_shape), dim=0)

                        logp_parts = torch.zeros([self._grid_action_parts])
                        mask_offset = 0
                        for a in range(self._grid_action_parts):
                            dist_part = dist_input_split[a]

                            sampled, logp, masked_part_logits, mask_part = self._mask_and_sample(
                                subtree_options,
                                dist_part
                            )

                            # Set the action and the mask for each part of the action
                            logit_end = mask_offset + self._grid_action_shape[a]
                            self._last_sample[i, a, x, y] = sampled
                            self._last_sample_masked_logits[i, mask_offset:logit_end, x, y] = masked_part_logits
                            self._last_sample_masks[i, mask_offset:logit_end, x, y] = mask_part

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
                        self._last_sample_logp[i] += torch.sum(logp_parts)

        return self._last_sample.flatten(1)

    def sampled_action_masks(self):
        return self._last_sample_masks.flatten(1)

    def sampled_masked_logits(self):
        return self._last_sample_masked_logits.flatten(1)

    def sampled_action_logp(self):
        return self._last_sample_logp
