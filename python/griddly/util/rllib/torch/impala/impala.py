from ray.rllib.agents.impala import ImpalaTrainer

from griddly.util.rllib.torch.impala.im_vtrace_torch_policy import InvalidMaskingVTraceTorchPolicy


def get_policy_class(config):
    if config['framework'] == 'torch':
        return InvalidMaskingVTraceTorchPolicy
    else:
        raise NotImplementedError('Tensorflow not supported')


InvalidActionMaskingImpalaTrainer = ImpalaTrainer.with_updates(default_policy=InvalidMaskingVTraceTorchPolicy,
                                                               get_policy_class=get_policy_class)