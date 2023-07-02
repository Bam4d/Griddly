import os
import shutil
import sys

import pytest
import ray
from griddly import gd
from griddly.util.rllib.callbacks import VideoCallbacks
from griddly.util.rllib.environment.core import (RLlibEnv,
                                                 RLlibMultiAgentWrapper)
from ray.rllib.algorithms.ppo import PPOConfig
from ray.rllib.models import ModelCatalog
from ray.rllib.models.torch.torch_modelv2 import TorchModelV2
from ray.tune import register_env, tune
from torch import nn


def count_videos(video_dir):
    count = 0
    for path in os.listdir(video_dir):
        # check if current path is a file
        if os.path.isfile(os.path.join(video_dir, path)):
            count += 1

    return count


class SingleAgentFlatModel(TorchModelV2, nn.Module):
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


@pytest.fixture(scope='module', autouse=True)
def ray_init():
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)
    ray.init(include_dashboard=False, num_cpus=1, num_gpus=0)

def test_rllib_single_player(test_name):
    

    register_env(test_name, lambda config: RLlibEnv(config))
    ModelCatalog.register_custom_model("SingleAgentFlatModel", SingleAgentFlatModel)

    test_dir = f"./testdir/{test_name}"

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=0, rollout_fragment_length=512)
        .training(
            model={"custom_model": "SingleAgentFlatModel"},
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
                "yaml_file": "Single-Player/GVGAI/sokoban.yaml",
            },
            env=test_name,
            clip_actions=True,
        )
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 100},
        storage_path=test_dir,
        config=config.to_dict(),
    )

    assert result is not None

    shutil.rmtree(test_dir)


@pytest.mark.skip(reason="ffmpeg not installed on test server")
def test_rllib_single_player_record_videos(test_name):
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)

    register_env(test_name, lambda config: RLlibEnv(config))
    ModelCatalog.register_custom_model("SingleAgentFlatModel", SingleAgentFlatModel)

    test_dir = f"./testdir/{test_name}"
    video_dir = "videos"

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=0, rollout_fragment_length=64)
        .callbacks(VideoCallbacks)
        .training(
            model={"custom_model": "SingleAgentFlatModel"},
            train_batch_size=64,
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
                "yaml_file": "Single-Player/GVGAI/sokoban.yaml",
                "max_steps": 50,
                "record_video_config": {"frequency": 100, "directory": video_dir},
            },
            env=test_name,
            clip_actions=True,
        )
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 512},
        storage_path=test_dir,
        config=config.to_dict(),
    )

    assert result is not None
    final_video_dir = os.path.join(result.trials[0].logdir, video_dir)
    assert count_videos(final_video_dir) > 0

    shutil.rmtree(test_dir)


class MultiAgentFlatModel(TorchModelV2, nn.Module):
    def __init__(self, obs_space, act_space, num_outputs, *args, **kwargs):
        TorchModelV2.__init__(self, obs_space, act_space, num_outputs, *args, **kwargs)
        nn.Module.__init__(self)
        self.model = nn.Sequential(
            nn.Flatten(),
            (nn.Linear(1458, 128)),
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

@pytest.mark.skip(reason="flaky on github actions")
def test_rllib_multi_agent_self_play(test_name):
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)

    register_env(
        test_name, lambda env_config: RLlibMultiAgentWrapper(RLlibEnv(env_config))
    )
    ModelCatalog.register_custom_model("MultiAgentFlatModel", MultiAgentFlatModel)

    test_dir = f"./testdir/{test_name}"

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=0, rollout_fragment_length=64)
        .training(
            model={"custom_model": "MultiAgentFlatModel"},
            train_batch_size=64,
            lr=2e-5,
            gamma=0.99,
            lambda_=0.9,
            use_gae=True,
            clip_param=0.4,
            grad_clip=None,
            entropy_coeff=0.1,
            vf_loss_coeff=0.25,
            sgd_minibatch_size=8,
            num_sgd_iter=10,
        )
        .environment(
            env_config={
                "global_observer_type": gd.ObserverType.VECTOR,
                "player_observer_type": gd.ObserverType.VECTOR,
                "yaml_file": "Multi-Agent/robot_tag_12.yaml",
            },
            env=test_name,
            clip_actions=True,
        )
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 512},
        storage_path=test_dir,
        config=config.to_dict(),
    )

    assert result is not None

    shutil.rmtree(test_dir)


@pytest.mark.skip(reason="ffmpeg not installed on test server")
def test_rllib_multi_agent_self_play_record_videos(test_name):
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)

    register_env(
        test_name, lambda env_config: RLlibMultiAgentWrapper(RLlibEnv(env_config))
    )
    ModelCatalog.register_custom_model("MultiAgentFlatModel", MultiAgentFlatModel)

    test_dir = f"./testdir/{test_name}"
    video_dir = "videos"

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=0, rollout_fragment_length=64)
        .callbacks(VideoCallbacks)
        .training(
            model={"custom_model": "MultiAgentFlatModel"},
            train_batch_size=64,
            lr=2e-5,
            gamma=0.99,
            lambda_=0.9,
            use_gae=True,
            clip_param=0.4,
            grad_clip=None,
            entropy_coeff=0.1,
            vf_loss_coeff=0.25,
            sgd_minibatch_size=8,
            num_sgd_iter=10,
        )
        .environment(
            env_config={
                "global_observer_type": gd.ObserverType.SPRITE_2D,
                "player_observer_type": gd.ObserverType.VECTOR,
                "yaml_file": "Multi-Agent/robot_tag_12.yaml",
                "record_video_config": {"frequency": 2, "directory": video_dir},
            },
            env=test_name,
            clip_actions=True,
        )
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 512},
        storage_path=test_dir,
        config=config.to_dict(),
    )

    assert result is not None
    final_video_dir = os.path.join(result.trials[0].logdir, video_dir)
    assert count_videos(final_video_dir) > 0

    shutil.rmtree(test_dir)


@pytest.mark.skip(reason="ffmpeg not installed on test server")
def test_rllib_multi_agent_self_play_record_videos_all_agents(test_name):
    sep = os.pathsep
    os.environ["PYTHONPATH"] = sep.join(sys.path)

    register_env(
        test_name, lambda env_config: RLlibMultiAgentWrapper(RLlibEnv(env_config))
    )
    ModelCatalog.register_custom_model("MultiAgentFlatModel", MultiAgentFlatModel)

    test_dir = f"./testdir/{test_name}"
    video_dir = "videos"

    config = (
        PPOConfig()
        .rollouts(num_rollout_workers=0, rollout_fragment_length=64)
        .callbacks(VideoCallbacks)
        .training(
            model={"custom_model": "MultiAgentFlatModel"},
            train_batch_size=64,
            lr=2e-5,
            gamma=0.99,
            lambda_=0.9,
            use_gae=True,
            clip_param=0.4,
            grad_clip=None,
            entropy_coeff=0.1,
            vf_loss_coeff=0.25,
            sgd_minibatch_size=8,
            num_sgd_iter=10,
        )
        .environment(
            env_config={
                "global_observer_type": gd.ObserverType.SPRITE_2D,
                "player_observer_type": gd.ObserverType.VECTOR,
                "yaml_file": "Multi-Agent/robot_tag_12.yaml",
                "player_done_variable": "player_done",
                "record_video_config": {
                    "frequency": 2,
                    "directory": video_dir,
                    "include_agents": True,
                    "include_global": True,
                },
                "max_steps": 200,
            },
            env=test_name,
            clip_actions=True,
        )
        .debugging(log_level="ERROR")
        .framework(framework="torch")
        .resources(num_gpus=int(os.environ.get("RLLIB_NUM_GPUS", "0")))
    )

    result = tune.run(
        "PPO",
        name="PPO",
        stop={"timesteps_total": 10000},
        storage_path=test_dir,
        config=config.to_dict(),
    )

    assert result is not None
    final_video_dir = os.path.join(result.trials[0].logdir, video_dir)
    assert count_videos(final_video_dir) > 0

    shutil.rmtree(test_dir)
