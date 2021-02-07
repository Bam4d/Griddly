import os
import sys

import ray
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy, setup_mixins
from ray.rllib.models import ModelCatalog
from ray.rllib.policy.torch_policy import LearningRateSchedule, EntropyCoeffSchedule
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib import RLlibMultiAgentWrapper
from griddly.util.rllib.torch import GAPAgent
from griddly.util.rllib.torch.mixins import InvalidActionMaskingPolicyMixin


if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(num_gpus=1)

    env_name = 'ray-griddly-rts-env'

    register_env(env_name, RLlibMultiAgentWrapper)
    ModelCatalog.register_custom_model('GAP', GAPAgent)


    def setup_invalid_mask_mixin(policy, obs_space, action_space, config):
        InvalidActionMaskingPolicyMixin.__init__(policy)
        setup_mixins(policy, obs_space, action_space, config)


    def get_policy_class(config):
        if config['framework'] == 'torch':
            return InvalidActionMaskingTorchPolicy
        else:
            raise NotImplementedError('Tensorflow not supported')


    InvalidActionMaskingTorchPolicy = VTraceTorchPolicy.with_updates(
        name='InvalidActionMaskingTorchPolicy',
        before_init=setup_invalid_mask_mixin,
        mixins=[
            LearningRateSchedule,
            EntropyCoeffSchedule,
            InvalidActionMaskingPolicyMixin
        ])


    InvalidActionMaskingImpalaTrainer = ImpalaTrainer.with_updates(default_policy=InvalidActionMaskingTorchPolicy,
                                                                   get_policy_class=get_policy_class)

    test_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'test_rts.yaml')

    config = {
        'framework': 'torch',
        'num_workers': 8,
        'num_envs_per_worker': 4,

        # Must be set to false to use the InvalidActionMaskingPolicyMixin
        "_use_trajectory_view_api": False,
        'monitor': True,
        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
             # Tell the RLlib wrapper to use invalid action masking
            'invalid_action_masking': True,

            'yaml_file': test_path,
            'global_observer_type': gd.ObserverType.SPRITE_2D,
            'level': 0,
            'max_steps': 1000,
        },
        'lr': tune.grid_search([0.0005])
    }

    stop = {
        # 'training_iteration': 100,
        'timesteps_total': 1000000,
    }

    result = tune.run(InvalidActionMaskingImpalaTrainer, config=config, stop=stop)
