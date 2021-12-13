import numpy as np
import gym
import pytest
from griddly import GymWrapperFactory, gd

@pytest.fixture
def test_name(request):
    return request.node.name


def build_test_env(test_name, yaml_file):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        yaml_file,
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )

    env = gym.make(f'GDY-{test_name}-v0')
    env.reset()
    return env

def test_state_hash_consistent_under_nop(test_name):
    """
    Test that under NOP actions, the state hash stays the same no state change other than environment steps
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    first_state = env.get_state()
    env.step(0)
    second_state = env.get_state()

    assert first_state['Hash'] == second_state['Hash']


def test_state_hash_consistent_return_to_state(test_name):
    """
    Test that if we move to one state and back, the two states should have consistent states
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    first_state_hashes = []
    second_state_hashes = []
    first_state_hashes.append(env.get_state()['Hash'])

    env.step(1)
    second_state_hashes.append(env.get_state()['Hash'])

    env.step(3)
    first_state_hashes.append(env.get_state()['Hash'])

    env.step(1)
    second_state_hashes.append(env.get_state()['Hash'])

    assert len(set(first_state_hashes)) == 1
    assert len(set(second_state_hashes)) == 1