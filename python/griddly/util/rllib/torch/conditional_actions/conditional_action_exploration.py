import torch
from gym.spaces import Discrete, MultiDiscrete
from ray.rllib.models.torch.torch_action_dist import TorchCategorical, TorchMultiCategorical
from torch.distributions import Categorical
import numpy as np

class TorchConditionalMaskingExploration():

    def __init__(self, model, dist_inputs, valid_action_trees, explore=False):
        self._valid_action_trees = valid_action_trees

        self._num_inputs = dist_inputs.shape[0]
        if isinstance(model.action_space, Discrete):
            self._action_space_shape = [model.action_space.n]
        elif isinstance(model.action_space, MultiDiscrete):
            self._action_space_shape = model.action_space.nvec

        self._num_action_logits = np.sum(self._action_space_shape)
        self._num_action_parts = len(self._action_space_shape)

        self._explore = explore

        self._inputs_split = dist_inputs.split(tuple(self._action_space_shape), dim=1)

    def _mask_and_sample(self, options, logits):

        mask = torch.zeros([logits.shape[0]])
        mask[options] = 1

        logits += torch.log(mask)
        dist = Categorical(logits=logits)
        sampled = dist.sample()
        logp = dist.log_prob(sampled)

        return sampled, logits, logp, mask

    def get_actions_and_mask(self):

        actions = torch.zeros([self._num_inputs, self._num_action_parts])
        masked_logits = torch.zeros([self._num_inputs, self._num_action_logits])
        mask = torch.zeros([self._num_inputs, self._num_action_logits])
        logp_sums = torch.zeros([self._num_inputs])

        if self._valid_action_trees is not None:

            for i in range(self._num_inputs):
                if len(self._valid_action_trees) >= 1:

                    subtree = self._valid_action_trees[i]
                    subtree_options = list(subtree.keys())

                    # In the case there are no available actions for the player
                    if len(subtree_options) == 0:
                        subtree = {}
                        for _ in range(self._num_action_parts):
                            subtree[0] = {}
                        subtree_options = [0]

                    logp_parts = torch.zeros([self._num_action_parts])
                    mask_offset = 0
                    for a in range(self._num_action_parts):
                        dist_part = self._inputs_split[a]
                        sampled, masked_part_logits, logp, mask_part = self._mask_and_sample(subtree_options, dist_part[i])

                        # Set the action and the mask for each part of the action
                        actions[i, a] = sampled
                        masked_logits[i, mask_offset:mask_offset + self._action_space_shape[a]] = masked_part_logits
                        mask[i, mask_offset:mask_offset + self._action_space_shape[a]] = mask_part

                        logp_parts[a] = logp

                        if mask_part.sum() == 0:
                            raise RuntimeError('mask calculated incorrectly')

                        mask_offset += self._action_space_shape[a]

                        if isinstance(subtree, dict):
                            subtree = subtree[int(sampled)]
                            if isinstance(subtree, dict):
                                subtree_options = list(subtree.keys())
                            else:
                                # Leaf nodes with action_id list
                                subtree_options = subtree

                    logp_sums[i] = torch.sum(logp_parts)

        # if its a discrete then flatten the space
        if self._num_action_parts == 1:
            actions = actions.flatten()

        return actions, masked_logits, logp_sums, mask