import pytest
import gymnasium as gym
from griddly import gd
from griddly.gym import GymWrapperFactory
from griddly.wrappers import RenderWrapper


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


def test_partial_observability_0_1(test_name):
    """
    Using observers in 0.1 version of GDY
    """
    env = build_test_env(
        test_name,
        "tests/gdy/partial_observability_0_1.yaml",
        global_observer_type="Vector",
        player_observer_type=gd.ObserverType.VECTOR,
    )

    global_observer = RenderWrapper(env, "global", "rgb_array")

    obs, reward, done, truncated, info = env.step([0, 0])
    player1_obs = obs[0]
    player2_obs = obs[1]

    assert env.player_observation_space[0].shape == (1, 3, 3)
    assert env.player_observation_space[1].shape == (1, 3, 3)
    assert env.global_observation_space.shape == (1, 5, 6)

    global_obs = global_observer.render()

    assert player1_obs.shape == (1, 3, 3)
    assert player2_obs.shape == (1, 3, 3)
    assert global_obs.shape == (60, 50, 3)


def test_partial_observability_0_2(test_name):
    """
    Using named observers in 0.2 version of GDY
    """
    env = build_test_env(
        test_name,
        "tests/gdy/partial_observability_0_2.yaml",
        global_observer_type="TestGlobalVectorObserver",
        player_observer_type=["TestVectorObserver", gd.ObserverType.VECTOR],
    )

    player_1_observer = RenderWrapper(env, 0, "rgb_array")
    player_2_observer = RenderWrapper(env, 1, "rgb_array")
    global_observer = RenderWrapper(env, "global", "rgb_array")

    obs, reward, done, truncated, info = env.step([0, 0])
    player1_obs = obs[0]
    player2_obs = obs[1]

    assert env.player_observation_space[0].shape == (4, 4, 4)
    assert env.player_observation_space[1].shape == (1, 4, 4)

    assert player_1_observer.observation_space.shape == (4, 4, 4)
    assert player_2_observer.observation_space.shape == (1, 4, 4)

    # Global named observers ignore the default observer options
    assert env.global_observation_space.shape == (1, 5, 6)
    assert global_observer.observation_space.shape == (1, 5, 6)

    global_rendered_obs = global_observer.render()
    player_1_rendered_obs = player_1_observer.render()
    player_2_rendered_obs = player_2_observer.render()

    assert player1_obs.shape == (4, 4, 4)
    assert player2_obs.shape == (1, 4, 4)

    assert global_rendered_obs.shape == (60, 50, 3)
    assert player_1_rendered_obs.shape == (40, 40, 3)
    assert player_2_rendered_obs.shape == (40, 40, 3)
