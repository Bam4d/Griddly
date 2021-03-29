from collections import defaultdict

import torch
from gym.spaces import Discrete, MultiDiscrete
from torch.distributions import Categorical
import numpy as np

class TorchConditionalMaskingExploration():

    def __init__(self, model, dist_inputs, valid_action_trees, explore=False, invalid_action_masking='conditional', allow_nop=False):
        self._valid_action_trees = valid_action_trees

        self._num_inputs = dist_inputs.shape[0]
        if isinstance(model.action_space, Discrete):
            self._action_space_shape = [model.action_space.n]
        elif isinstance(model.action_space, MultiDiscrete):
            self._action_space_shape = model.action_space.nvec

        self._num_action_logits = np.sum(self._action_space_shape)
        self._num_action_parts = len(self._action_space_shape)

        self._invalid_action_masking = invalid_action_masking
        self._allow_nop = allow_nop

        self._explore = explore

        self._inputs_split = dist_inputs.split(tuple(self._action_space_shape), dim=1)

        self._full_tree = self._fill_node(self._action_space_shape,0)

    def _mask_and_sample(self, options, logits, is_parameters=False):

        mask = torch.zeros([logits.shape[0]]).to(logits.device)
        mask[options] = 1

        if is_parameters:
            if not self._allow_nop and len(options) > 1:
                mask[0] = 0

        masked_logits = logits + torch.log(mask)

        dist = Categorical(logits=masked_logits)
        sampled = dist.sample()
        logp = dist.log_prob(sampled)
        out_logits = masked_logits

        # if not self._allow_nop and is_parameters:
        #     assert sampled != 0


        return sampled, out_logits, logp, mask

    def _fill_node(self, keys, pos):
        if pos < len(keys):
            return {k: self._fill_node(keys, pos + 1) for k in np.arange(keys[pos])}
        else:
            return {}

    def _merge_all_branches(self, tree):
        all_nodes = {}
        merged_tree = {}
        for k, v in tree.items():
            v = self._merge_all_branches(v)
            all_nodes.update(v)

        for k in tree.keys():
            merged_tree[k] = all_nodes

        return merged_tree

    def _process_valid_action_tree(self, valid_action_tree):
        subtree = valid_action_tree
        subtree_options = list(subtree.keys())

        # In the case there are no available actions for the player
        if len(subtree_options) == 0:
            #subtree = self._full_tree
            build_tree = subtree
            for _ in range(self._num_action_parts):
                build_tree[0] = {}
                build_tree = build_tree[0]
            subtree_options = list(subtree.keys())

        # If we want very basic action masking where parameterized masks are superimposed we use this
        if self._invalid_action_masking == 'collapsed':
            subtree = self._merge_all_branches(valid_action_tree)
            subtree_options = list(subtree.keys())

        return subtree, subtree_options

    def get_actions_and_mask(self):

        actions = torch.zeros([self._num_inputs, self._num_action_parts])
        masked_logits = torch.zeros([self._num_inputs, self._num_action_logits])
        mask = torch.zeros([self._num_inputs, self._num_action_logits])
        logp_sums = torch.zeros([self._num_inputs])

        if self._valid_action_trees is not None:

            for i in range(self._num_inputs):
                if len(self._valid_action_trees) >= 1:

                    subtree, subtree_options = self._process_valid_action_tree(self._valid_action_trees[i])

                    logp_parts = torch.zeros([self._num_action_parts])
                    mask_offset = 0
                    for a in range(self._num_action_parts):

                        try:
                            dist_part = self._inputs_split[a]
                            is_parameters = a==(self._num_action_parts-1)
                            sampled, masked_part_logits, logp, mask_part = self._mask_and_sample(subtree_options, dist_part[i], is_parameters)

                            # Set the action and the mask for each part of the action
                            actions[i, a] = sampled
                            masked_logits[i, mask_offset:mask_offset + self._action_space_shape[a]] = masked_part_logits
                            mask[i, mask_offset:mask_offset + self._action_space_shape[a]] = mask_part

                            logp_parts[a] = logp

                            mask_offset += self._action_space_shape[a]

                            subtree = subtree[int(sampled)]
                            subtree_options = list(subtree.keys())
                        except ValueError as e:
                            print(e)


                    logp_sums[i] = torch.sum(logp_parts)

        # if its a discrete then flatten the space
        if self._num_action_parts == 1:
            actions = actions.flatten()

        return actions, masked_logits, logp_sums, mask