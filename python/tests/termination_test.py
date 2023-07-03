import numpy as np
import gymnasium as gym
import pytest
from griddly import GymWrapperFactory, gd


@pytest.fixture
def test_name(request):
    return request.node.name


def build_test_env(test_name, yaml_file, **kwargs):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        yaml_file,
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
        **kwargs,
    )

    env = gym.make(f"GDY-{test_name}-v0")
    env.reset()
    return env


def test_termination_steps(test_name):
    """
    Test that when we reach 100 steps the environment resets
    """

    env = build_test_env(test_name, "tests/gdy/test_termination_steps.yaml")

    for i in range(102):
        obs, reward, done, truncated, info = env.step(0)

        if done:
            print(f"Environment done at {i} steps")

        if i == 99:
            assert done, "environment should be reset"
            assert reward == -1, "Reward should be -1"
            break


def test_override_termination_steps(test_name):
    """
    Test that when we reach 50 steps, (as overridden) the environment resets
    """

    env = build_test_env(
        test_name, "tests/gdy/test_termination_steps.yaml", max_steps=50
    )

    for i in range(102):
        obs, reward, done, truncated, info = env.step(0)

        if done:
            print(f"Environment done at {i} steps")

        if i == 50:
            assert done, "environment should be reset"
            break
