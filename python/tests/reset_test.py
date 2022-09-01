import numpy as np
import gym
import pytest
from gym.spaces import Discrete

from griddly import GymWrapperFactory, gd


@pytest.fixture
def test_name(request):
    return request.node.name


def build_env(test_name, yaml_file, global_observations=False):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        yaml_file,
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )

    return gym.make(f"GDY-{test_name}-v0")


def test_reset_SinglePlayer(test_name):
    """
    Only a single player, reset should return:

    reset() # returns just player obs
    np.array([player observations])

    """
    env = build_env(test_name, "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml")
    obs = env.reset()

    assert isinstance(obs, np.ndarray)
    assert obs.shape == (1, 5, 6)
    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == ()
    assert env.action_space.n == 5

    obs = env.reset()

    assert isinstance(obs, np.ndarray)
    assert obs.shape == (1, 5, 6)
    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == ()
    assert env.action_space.n == 5


def test_reset_SinglePlayer_Global(test_name):
    """
    Only a single player, reset should return:

    reset(global_observation=True) # returns both global and player
    {
        'global': np.array([global observation]),
        'player': np.array([player observation])
    }

    """

    env = build_env(test_name, "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml")
    obs = env.reset(options={"global_observations": True})

    global_obs = obs["global"]
    player_obs = obs["player"]

    assert isinstance(player_obs, np.ndarray)
    assert isinstance(global_obs, np.ndarray)
    assert player_obs.shape == (1, 5, 6)
    assert global_obs.shape == (1, 5, 6)
    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == ()
    assert env.action_space.n == 5

    obs = env.reset(options={"global_observations": True})

    global_obs = obs["global"]
    player_obs = obs["player"]

    assert isinstance(player_obs, np.ndarray)
    assert isinstance(global_obs, np.ndarray)
    assert player_obs.shape == (1, 5, 6)
    assert global_obs.shape == (1, 5, 6)
    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == ()
    assert env.action_space.n == 5


def test_reset_MultiPlayer(test_name):
    """
    Multiple players. reset should return:

    reset() # returns just player obs
    [
      np.array([player1 observations]),
      np.array([player2 observations])
    ]

    """

    env = build_env(test_name, "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml")
    obs = env.reset()

    for p in range(env.player_count):
        assert isinstance(obs[p], np.ndarray)
        assert obs[p].shape == (1, 5, 6)
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == ()
        assert isinstance(env.action_space[p], Discrete)
        assert env.action_space[p].n == 5

    obs = env.reset()

    for p in range(env.player_count):
        assert isinstance(obs[p], np.ndarray)
        assert obs[p].shape == (1, 5, 6)
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == ()
        assert isinstance(env.action_space[p], Discrete)
        assert env.action_space[p].n == 5


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

    env = build_env(test_name, "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml")
    obs = env.reset(options={"global_observations": True})

    global_obs = obs["global"]
    player_obs = obs["player"]

    assert isinstance(global_obs, np.ndarray)
    assert global_obs.shape == (1, 5, 6)

    for p in range(env.player_count):
        assert isinstance(player_obs[0], np.ndarray)
        assert player_obs[0].shape == (1, 5, 6)
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == ()
        assert isinstance(env.action_space[p], Discrete)
        assert env.action_space[p].n == 5

    obs = env.reset(options={"global_observations": True})

    global_obs = obs["global"]
    player_obs = obs["player"]

    assert isinstance(global_obs, np.ndarray)
    assert global_obs.shape == (1, 5, 6)

    for p in range(env.player_count):
        assert isinstance(player_obs[0], np.ndarray)
        assert player_obs[0].shape == (1, 5, 6)
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == ()
        assert isinstance(env.action_space[p], Discrete)
        assert env.action_space[p].n == 5


def test_reset_MultiPlayer_MultiActionSpace(test_name):
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

    env = build_env(
        test_name, "tests/gdy/test_step_MultiPlayer_MultipleActionType.yaml"
    )
    obs = env.reset(options={"global_observations": True})

    global_obs = obs["global"]
    player_obs = obs["player"]

    assert isinstance(global_obs, np.ndarray)
    assert global_obs.shape == (1, 5, 6)

    for p in range(env.player_count):
        assert isinstance(player_obs[0], np.ndarray)
        assert player_obs[0].shape == (1, 5, 6)
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == (2,)
        assert np.all(env.action_space[p].nvec == [2, 5])

    obs = env.reset(options={"global_observations": True})

    global_obs = obs["global"]
    player_obs = obs["player"]

    assert isinstance(global_obs, np.ndarray)
    assert global_obs.shape == (1, 5, 6)

    for p in range(env.player_count):
        assert isinstance(player_obs[0], np.ndarray)
        assert player_obs[0].shape == (1, 5, 6)
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == (2,)
        assert np.all(env.action_space[p].nvec == [2, 5])
