import gymnasium as gym
import pytest

from griddly import GymWrapperFactory


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


def test_vector1(test_name):
    """
    Test that we get a 10x10 observation space
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_observer.yaml",
        global_observer_type="None",
        player_observer_type="Vector1",
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step(0)

    assert env.player_observation_space.shape == (1, 10, 10)
    assert obs.shape == (1, 10, 10)
    assert init_obs.shape == (1, 10, 10)


def test_vector2(test_name):
    """
    Test that we get a 5x5 observation space
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_observer.yaml",
        global_observer_type="None",
        player_observer_type="Vector2",
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step(0)

    assert env.player_observation_space.shape == (1, 5, 5)
    assert obs.shape == (1, 5, 5)
    assert init_obs.shape == (1, 5, 5)


def test_vector3(test_name):
    """
    Test that we get a 4x4 observation space
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_observer.yaml",
        global_observer_type="None",
        player_observer_type="Vector3",
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step(0)

    assert env.player_observation_space.shape == (1, 4, 4)
    assert obs.shape == (1, 4, 4)
    assert init_obs.shape == (1, 4, 4)


def test_multi_object_vector1(test_name):
    """
    Test that we get a 10x10 observation space for both players
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_agent_multi_observer.yaml",
        global_observer_type="None",
        player_observer_type="Vector1",
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step([0, 0])

    assert env.observation_space[0].shape == (1, 10, 10)
    assert env.observation_space[1].shape == (1, 10, 10)
    assert env.player_observation_space[0].shape == (1, 10, 10)
    assert env.player_observation_space[1].shape == (1, 10, 10)
    assert obs[0].shape == (1, 10, 10)
    assert obs[1].shape == (1, 10, 10)
    assert init_obs[0].shape == (1, 10, 10)
    assert init_obs[1].shape == (1, 10, 10)


def test_multi_object_vector2(test_name):
    """
    Test that we get a 5x5 observation space for both players
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_agent_multi_observer.yaml",
        global_observer_type="None",
        player_observer_type="Vector2",
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step([0, 0])

    assert env.observation_space[0].shape == (1, 5, 5)
    assert env.observation_space[1].shape == (1, 5, 5)
    assert env.player_observation_space[0].shape == (1, 5, 5)
    assert env.player_observation_space[1].shape == (1, 5, 5)
    assert obs[0].shape == (1, 5, 5)
    assert obs[1].shape == (1, 5, 5)
    assert init_obs[0].shape == (1, 5, 5)
    assert init_obs[1].shape == (1, 5, 5)


def test_multi_object_vector3(test_name):
    """
    Test that we get a 4x4 observation space for both players
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_agent_multi_observer.yaml",
        global_observer_type="None",
        player_observer_type="Vector3",
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step([0, 0])

    assert env.observation_space[0].shape == (1, 4, 4)
    assert env.observation_space[1].shape == (1, 4, 4)
    assert env.player_observation_space[0].shape == (1, 4, 4)
    assert env.player_observation_space[1].shape == (1, 4, 4)
    assert obs[0].shape == (1, 4, 4)
    assert obs[1].shape == (1, 4, 4)
    assert init_obs[0].shape == (1, 4, 4)
    assert init_obs[1].shape == (1, 4, 4)


def test_multi_object_vector1_vector2(test_name):
    """
    Test that we get a 10x10 observation space for player 1 and 5x5 for player 2
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_agent_multi_observer.yaml",
        global_observer_type="None",
        player_observer_type=["Vector1", "Vector2"],
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step([0, 0])

    assert env.observation_space[0].shape == (1, 10, 10)
    assert env.observation_space[1].shape == (1, 5, 5)
    assert env.player_observation_space[0].shape == (1, 10, 10)
    assert env.player_observation_space[1].shape == (1, 5, 5)
    assert obs[0].shape == (1, 10, 10)
    assert obs[1].shape == (1, 5, 5)
    assert init_obs[0].shape == (1, 10, 10)
    assert init_obs[1].shape == (1, 5, 5)


def test_multi_object_vector2_vector3(test_name):
    """
    Test that we get a 5x5 observation space for player 1 and 4x4 for player 2
    """
    env = build_test_env(
        test_name,
        "tests/gdy/multi_agent_multi_observer.yaml",
        global_observer_type="None",
        player_observer_type=["Vector2", "Vector3"],
    )

    init_obs, init_info = env.reset()

    obs, reward, done, truncated, info = env.step([0, 0])

    assert env.observation_space[0].shape == (1, 5, 5)
    assert env.observation_space[1].shape == (1, 4, 4)
    assert env.player_observation_space[0].shape == (1, 5, 5)
    assert env.player_observation_space[1].shape == (1, 4, 4)
    assert obs[0].shape == (1, 5, 5)
    assert obs[1].shape == (1, 4, 4)
    assert init_obs[0].shape == (1, 5, 5)
    assert init_obs[1].shape == (1, 4, 4)
