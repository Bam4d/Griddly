from ray.rllib.agents import with_common_config
from ray.rllib.agents.impala import ImpalaTrainer, DEFAULT_CONFIG as IMPALA_CONFIG
from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy
from ray.rllib.policy.torch_policy import LearningRateSchedule, EntropyCoeffSchedule

from griddly.util.rllib.torch.conditional_actions.conditional_action_mixin import ConditionalActionMixin


def setup_mixins(policy, obs_space, action_space, config):
    ConditionalActionMixin.__init__(policy)
    EntropyCoeffSchedule.__init__(policy, config["entropy_coeff"],
                                  config["entropy_coeff_schedule"])
    LearningRateSchedule.__init__(policy, config["lr"], config["lr_schedule"])


ConditionalActionVTraceTorchPolicy = VTraceTorchPolicy.with_updates(
    name="ConditionalActionVTraceTorchPolicy",
    before_init=setup_mixins,
    mixins=[LearningRateSchedule, EntropyCoeffSchedule, ConditionalActionMixin]
)


def get_vtrace_policy_class(config):
    if config['framework'] == 'torch':
        return ConditionalActionVTraceTorchPolicy
    else:
        raise NotImplementedError('Tensorflow not supported')


ConditionalActionImpalaTrainer = ImpalaTrainer.with_updates(default_policy=ConditionalActionVTraceTorchPolicy,
                                                            get_policy_class=get_vtrace_policy_class)
