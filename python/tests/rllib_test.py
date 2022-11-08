import os
import sys

import pytest
from ray.rllib.algorithms.ppo import PPOConfig
from ray.rllib.models import ModelCatalog
from ray.rllib.models.torch.torch_modelv2 import TorchModelV2
from ray.tune import register_env, tune
from torch import nn

from griddly import gd
from griddly.util.rllib.environment.core import RLlibEnv, RLlibMultiAgentWrapper


class FlatModel(TorchModelV2, nn.Module):
    def __init__(self, obs_space, act_space, num_outputs, *args, **kwargs):
        TorchModelV2.__init__(self, obs_space, act_space, num_outputs, *args, **kwargs)
        nn.Module.__init__(self)
        self.model = nn.Sequential(
            nn.Flatten(),
            (nn.Linear(468, 128)),
            nn.ReLU(),
        )
        self.policy_fn = nn.Linear(128, num_outputs)
        self.value_fn = nn.Linear(128, 1)

    def forward(self, input_dict, state, seq_lens):
        model_out = self.model(input_dict["obs"].permute(0, 3, 1, 2))
        self._value_out = self.value_fn(model_out)
        return self.policy_fn(model_out), state

    def value_function(self):
        return self._value_out.flatten()


@pytest.fixture
def test_name(request):
    return request.node.name

def test_rllib_single_player(test_name):
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)

    register_env(test_name, lambda config: RLlibEnv(config))
    ModelCatalog.register_custom_model("FlatModel", FlatModel)

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=1, rollout_fragment_length=512)
        .training(
            model={
                "custom_model": "FlatModel"
            },
            train_batch_size=512,
            lr=2e-5,
            gamma=0.99,
            lambda_=0.9,
            use_gae=True,
            clip_param=0.4,
            grad_clip=None,
            entropy_coeff=0.1,
            vf_loss_coeff=0.25,
            sgd_minibatch_size=64,
            num_sgd_iter=10,
        )
        .environment(
            env_config={
                "global_observer_type": gd.ObserverType.VECTOR,
                "player_observer_type": gd.ObserverType.VECTOR,
                "yaml_file": "Single-Player/GVGAI/sokoban.yaml"
            },
            env=test_name, clip_actions=True)
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 100},
        local_dir="~/ray_results/" + test_name,
        config=config.to_dict(),
    )

    assert result is not None

def test_rllib_multi_agent_self_play(test_name):
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)

    register_env(test_name, lambda env_config: RLlibMultiAgentWrapper(RLlibEnv(env_config)))
    ModelCatalog.register_custom_model("FlatModel", FlatModel)

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=1, rollout_fragment_length=512)
        .training(
            model={
                "custom_model": "FlatModel"
            },
            train_batch_size=512,
            lr=2e-5,
            gamma=0.99,
            lambda_=0.9,
            use_gae=True,
            clip_param=0.4,
            grad_clip=None,
            entropy_coeff=0.1,
            vf_loss_coeff=0.25,
            sgd_minibatch_size=64,
            num_sgd_iter=10,
        )
        .environment(
            env_config={
                "global_observer_type": gd.ObserverType.VECTOR,
                "player_observer_type": gd.ObserverType.VECTOR,
                "yaml_file": "Multi-Agent/robot_tag_12.yaml",
            },
            env=test_name, clip_actions=True)
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 100},
        local_dir="~/ray_results/" + test_name,
        config=config.to_dict(),
    )

    assert result is not None