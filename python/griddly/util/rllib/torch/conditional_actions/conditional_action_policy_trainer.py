import gym
import numpy as np
import torch
from ray.rllib import SampleBatch
from ray.rllib.agents.impala import ImpalaTrainer
from ray.rllib.agents.impala.vtrace_torch_policy import VTraceTorchPolicy, VTraceLoss, make_time_major
from ray.rllib.models.torch.torch_action_dist import TorchCategorical
from ray.rllib.policy.torch_policy import LearningRateSchedule, EntropyCoeffSchedule
from ray.rllib.utils.torch_ops import sequence_mask

from griddly.util.rllib.torch.conditional_actions.conditional_action_mixin import ConditionalActionMixin


def build_invalid_masking_vtrace_loss(policy, model, dist_class, train_batch):

    model_out, _ = model.from_batch(train_batch)

    if isinstance(policy.action_space, gym.spaces.Discrete):
        is_multidiscrete = False
        output_hidden_shape = [policy.action_space.n]
    elif isinstance(policy.action_space, gym.spaces.MultiDiscrete):
        is_multidiscrete = True
        output_hidden_shape = policy.action_space.nvec.astype(np.int32)
    else:
        is_multidiscrete = False
        output_hidden_shape = 1

    def _make_time_major(*args, **kw):
        return make_time_major(policy, train_batch.get("seq_lens"), *args,
                               **kw)

    actions = train_batch[SampleBatch.ACTIONS]
    dones = train_batch[SampleBatch.DONES]
    rewards = train_batch[SampleBatch.REWARDS]
    behaviour_action_logp = train_batch[SampleBatch.ACTION_LOGP]
    behaviour_logits = train_batch[SampleBatch.ACTION_DIST_INPUTS]

    invalid_action_mask = train_batch['invalid_action_mask']

    if 'seq_lens' in train_batch:
        max_seq_len = policy.config['rollout_fragment_length']
        mask_orig = sequence_mask(train_batch["seq_lens"], max_seq_len)
        mask = torch.reshape(mask_orig, [-1])
    else:
        mask = torch.ones_like(rewards)

    model_out += torch.maximum(torch.tensor(torch.finfo().min), torch.log(invalid_action_mask))
    action_dist = dist_class(model_out, model)

    if isinstance(output_hidden_shape, (list, tuple, np.ndarray)):
        unpacked_behaviour_logits = torch.split(
            behaviour_logits, list(output_hidden_shape), dim=1)
        unpacked_outputs = torch.split(
            model_out, list(output_hidden_shape), dim=1)
    else:
        unpacked_behaviour_logits = torch.chunk(
            behaviour_logits, output_hidden_shape, dim=1)
        unpacked_outputs = torch.chunk(model_out, output_hidden_shape, dim=1)
    values = model.value_function()

    # Prepare actions for loss.
    loss_actions = actions if is_multidiscrete else torch.unsqueeze(
        actions, dim=1)

    # Inputs are reshaped from [B * T] => [T - 1, B] for V-trace calc.
    policy.loss = VTraceLoss(
        actions=_make_time_major(loss_actions, drop_last=True),
        actions_logp=_make_time_major(
            action_dist.logp(actions), drop_last=True),
        actions_entropy=_make_time_major(
            action_dist.entropy(), drop_last=True),
        dones=_make_time_major(dones, drop_last=True),
        behaviour_action_logp=_make_time_major(
            behaviour_action_logp, drop_last=True),
        behaviour_logits=_make_time_major(
            unpacked_behaviour_logits, drop_last=True),
        target_logits=_make_time_major(unpacked_outputs, drop_last=True),
        discount=policy.config["gamma"],
        rewards=_make_time_major(rewards, drop_last=True),
        values=_make_time_major(values, drop_last=True),
        bootstrap_value=_make_time_major(values)[-1],
        dist_class=TorchCategorical if is_multidiscrete else dist_class,
        model=model,
        valid_mask=_make_time_major(mask, drop_last=True),
        config=policy.config,
        vf_loss_coeff=policy.config["vf_loss_coeff"],
        entropy_coeff=policy.entropy_coeff,
        clip_rho_threshold=policy.config["vtrace_clip_rho_threshold"],
        clip_pg_rho_threshold=policy.config["vtrace_clip_pg_rho_threshold"])

    return policy.loss.total_loss


def setup_mixins(policy, obs_space, action_space, config):
    ConditionalActionMixin.__init__(policy)
    EntropyCoeffSchedule.__init__(policy, config["entropy_coeff"],
                                  config["entropy_coeff_schedule"])
    LearningRateSchedule.__init__(policy, config["lr"], config["lr_schedule"])


ConditionalActionVTraceTorchPolicy = VTraceTorchPolicy.with_updates(
    name="ConditionalActionVTraceTorchPolicy",
    loss_fn=build_invalid_masking_vtrace_loss,
    before_init=setup_mixins,
    mixins=[LearningRateSchedule, EntropyCoeffSchedule, ConditionalActionMixin]
)


def get_vtrace_policy_class(config):
    if config['framework'] == 'torch':
        return ConditionalActionVTraceTorchPolicy
    else:
        raise NotImplementedError('Tensorflow not supported')


ConditionalActionImpalaTrainer = ImpalaTrainer.with_updates(name="ConditionalActionImpalaTrainer",
                                                            default_policy=ConditionalActionVTraceTorchPolicy,
                                                            get_policy_class=get_vtrace_policy_class)
