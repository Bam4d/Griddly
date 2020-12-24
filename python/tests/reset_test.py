import numpy as np
import gym
import pytest
from griddly import GymWrapperFactory, gd


@pytest.fixture
def test_name(request):
    return request.node.name


def build_and_reset(test_name, yaml_file, global_observations=False):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        yaml_file,
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )

    env = gym.make(f'GDY-{test_name}-v0')
    return env.reset(global_observations=global_observations)


def test_reset_SinglePlayer(test_name):
    """
    Only a single player, reset should return:

    reset() # returns just player obs
    np.array([player observations])

    """

    obs = build_and_reset(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    assert isinstance(obs, np.ndarray)
    assert obs.shape == (1, 5, 6)


def test_reset_SinglePlayer_Global(test_name):
    """
    Only a single player, reset should return:

    reset(global_observation=True) # returns both global and player
    {
        'global': np.array([global observation]),
        'player': np.array([player observation])
    }

    """

    obs = build_and_reset(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml",
        global_observations=True
    )

    global_obs = obs['global']
    player_obs = obs['player']

    assert isinstance(player_obs, np.ndarray)
    assert player_obs.shape == (1, 5, 6)

    assert isinstance(global_obs, np.ndarray)
    assert global_obs.shape == (1, 5, 6)


def test_reset_MultiPlayer(test_name):
    """
    Multiple players. reset should return:

    reset() # returns just player obs
    [
      np.array([player1 observations]),
      np.array([player2 observations])
    ]

    """

    obs = build_and_reset(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml"
    )

    for p in range(2):
        assert isinstance(obs[0], np.ndarray)
        assert obs[0].shape == (1, 5, 6)

def test_reset_MultiPlayer_Global(test_name):
    """
    Multiple players. reset should return:

    reset(global_observation=True) # returns both global and player
    {
        'global': np.array([global observation]),
        'player': [
          np.array([player1 observations]),
          np.array([player2 observations])
        ]
    }
    """

    obs = build_and_reset(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml",
        global_observations=True
    )

    global_obs = obs['global']
    player_obs = obs['player']

    assert isinstance(global_obs, np.ndarray)
    assert global_obs.shape == (1, 5, 6)

    for p in range(2):
        assert isinstance(player_obs[0], np.ndarray)
        assert player_obs[0].shape == (1, 5, 6)