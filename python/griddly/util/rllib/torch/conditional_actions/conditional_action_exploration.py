from collections import defaultdict

import torch
from gym.spaces import Discrete, MultiDiscrete
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

        self.model = model

        self.device = dist_inputs.device

        self._explore = explore

        self._inputs_split = dist_inputs.split(tuple(self._action_space_shape), dim=1)

    def _mask_and_sample(self, subtrees, logits):

        mask = torch.zeros_like(logits).to(self.device)
        for i in range(self._num_inputs):
            try:
                mask[i][list(subtrees[i].keys())] = 1
            except IndexError as e:
                print(e)
                print(list(subtrees[i].keys()))
                print(subtrees)
                raise e

        masked_logits = logits + torch.log(mask)

        dist = Categorical(logits=masked_logits)
        sampled = dist.sample()
        logp = dist.log_prob(sampled)
        out_logits = masked_logits

        next_subtrees = []
        for i in range(self._num_inputs):
            next_subtrees.append(subtrees[i][int(sampled[i])])

        return sampled, next_subtrees, out_logits, logp, mask

    def _process_valid_action_tree_batched(self, valid_action_tree):
        subtree = valid_action_tree

        # In the case there are no available actions for the player
        if len(subtree.keys()) == 0:
            build_tree = subtree
            for _ in range(self._num_action_parts):
                build_tree[0] = {}
                build_tree = build_tree[0]

        return subtree

    def get_actions_and_mask(self):

        actions = torch.zeros([self._num_inputs, self._num_action_parts]).to(self.device)
        masked_logits = torch.zeros([self._num_inputs, self._num_action_logits]).to(self.device)
        mask = torch.zeros([self._num_inputs, self._num_action_logits]).to(self.device)
        logp_sums = torch.zeros([self._num_inputs]).to(self.device)

        if self._valid_action_trees is not None:

            if len(self._valid_action_trees) >= 1:

                subtrees = [self._process_valid_action_tree_batched(tree) for tree in self._valid_action_trees]
                mask_offset = 0
                for a in range(self._num_action_parts):
                    dist_part = self._inputs_split[a]

                    sampled, subtrees, masked_part_logits, logp, mask_part = self._mask_and_sample(subtrees,
                                                                                                   dist_part)

                    # Set the action and the mask for each part of the action
                    actions[:, a] = sampled
                    masked_logits[:, mask_offset:mask_offset + self._action_space_shape[a]] = masked_part_logits
                    mask[:, mask_offset:mask_offset + self._action_space_shape[a]] = mask_part

                    logp_sums += logp

                    mask_offset += self._action_space_shape[a]

        # if its a discrete then flatten the space
        if self._num_action_parts == 1:
            actions = actions.flatten()

        return actions, masked_logits, logp_sums, mask
