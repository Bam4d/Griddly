from typing import Union

from ray.rllib.models import ActionDistribution
from ray.rllib.utils.exploration import StochasticSampling
from ray.rllib.utils.framework import TensorType


class InvalidActionMaskingPolicyWrapper():

    @staticmethod
    def action_sampler_fn(policy, model, input_dict, state_out, explore, timestep):

        location_mask = input_dict['location_action_mask']
        action_masks = input_dict['unit_action_mask']

        # Firstly sample from the location mask


        # Use the sampled location to retrieve the unit action mask


    @staticmethod
    def wrap(policy_cls, name):
        return policy_cls.with_updates(name, action_sampler_fn=InvalidActionMaskingPolicyWrapper.action_sampler_fn)
