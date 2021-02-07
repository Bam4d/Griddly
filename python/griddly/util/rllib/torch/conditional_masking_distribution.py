from typing import Union
import numpy as np
import torch
from torch.distributions import Categorical


class TorchConditionalMaskingExploration():

    def __init__(self, dist_inputs, valid_action_trees, dist_class):
        self._valid_action_trees = valid_action_trees
        self._dist_class = dist_class

        self._num_inputs = dist_inputs.shape[0]
        self._action_space_shape = dist_class.keywords['input_lens']
        self._num_action_logits = np.sum(self._action_space_shape)
        self._num_action_parts = len(self._action_space_shape)

        self._inputs_split = dist_inputs.split(tuple(self._action_space_shape), dim=1)

    def _mask_and_sample(self, options, logits):
        mask = torch.zeros([logits.shape[0]])
        mask[options] = 1

        logits += torch.log(mask)
        dist = Categorical(logits=logits)
        sampled = dist.sample()

        return sampled, logits

    def get_actions_and_mask(self):

        actions = torch.zeros([self._num_inputs, self._num_action_parts])
        masked_logits = torch.zeros([self._num_inputs, self._num_action_logits])

        for i in range(self._num_inputs):
            # just do nothing if we have no action tree, also no gradients are propagated because mask is 0
            if len(self._valid_action_trees) >= 1:

                subtree = self._valid_action_trees[i]
                subtree_options = list(subtree.keys())
                mask_offset = 0
                for a in range(self._num_action_parts):
                    dist_part = self._inputs_split[a]
                    sampled, masked_logits_part = self._mask_and_sample(subtree_options, dist_part[i])

                    # Set the action and the mask for each part of the action
                    actions[i, a] = sampled
                    masked_logits[i, mask_offset:mask_offset + self._action_space_shape[a]] = masked_logits_part

                    if isinstance(subtree, dict):
                        subtree = subtree[int(sampled)]
                        if isinstance(subtree, dict):
                            subtree_options = list(subtree.keys())
                        else:
                            # Leaf nodes with action_id list
                            subtree_options = subtree

        return actions, masked_logits
