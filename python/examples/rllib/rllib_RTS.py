import os
import sys

import ray
from ray import tune
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.agents.impala.vtrace_tf_policy import VTraceTFPolicy
from ray.rllib.models import ModelCatalog, ActionDistribution
from ray.tune.registry import register_env

from griddly import gd
from griddly.util.rllib import RLlibMultiAgentWrapper
from griddly.util.rllib.torch import GAPAgent



if __name__ == '__main__':
    sep = os.pathsep
    os.environ['PYTHONPATH'] = sep.join(sys.path)

    ray.init(local_mode=True)

    env_name = 'ray-griddly-rts-env'

    register_env(env_name, RLlibMultiAgentWrapper)
    ModelCatalog.register_custom_model('GAP', GAPAgent)

    #ActionDistribution.with_updates()
    InvalidActionMaskingPolicyWrapper(VTraceTFPolicy, 'InvalidActionMaskingVTrace')

    config = {
        'framework': 'torch',
        'num_workers': 1,
        'num_envs_per_worker': 1,
        #'monitor': True,
        'model': {
            'custom_model': 'GAP',
            'custom_model_config': {}
        },
        'env': env_name,
        'env_config': {
            # Tell the RLlib wrapper to use invalid action masking
            #'invalid_action_masking': True,

            'yaml_file': 'RTS/Stratega/heal-or-die.yaml',
            #'global_observer_type': gd.ObserverType.SPRITE_2D,
            'level': 0,
            'max_steps': 1000,
        },
        'lr': tune.grid_search([0.0005])
    }

    stop = {
        # 'training_iteration': 100,
        'timesteps_total': 1000000,
    }

    result = tune.run(ImpalaTrainer, config=config, stop=stop)
