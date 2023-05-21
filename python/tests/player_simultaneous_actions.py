import pytest
import gymnasium as gym
from griddly import GymWrapperFactory, gd

from collections import Counter


@pytest.fixture
def test_name(request):
    return request.node.name


def build_test_env(test_name, yaml_file, **kwargs):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        yaml_file,
        **kwargs,
    )

    env = gym.make(f"GDY-{test_name}-v0")
    env.reset()
    return env


def test_simultaneous_actions_stochasticity(test_name):
    """
    Using observers in 0.1 version of GDY
    """
    env = build_test_env(test_name, "tests/gdy/simultaneous_player_actions.yaml", global_observer_type=gd.ObserverType.VECTOR,
                         player_observer_type=gd.ObserverType.VECTOR)

    possible_obs_hashes = Counter()

    for i in range(100):
        obs, reward, done, info = env.step([3, 1])
        obs_hash_player_1 = hash(tuple(obs[0].flatten()))
        obs_hash_player_2 = hash(tuple(obs[1].flatten()))
        possible_obs_hashes[obs_hash_player_1] += 1
        possible_obs_hashes[obs_hash_player_2] += 1
        env.reset()

    assert len(possible_obs_hashes) == 4


