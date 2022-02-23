import pytest
import gym
from griddly import GymWrapperFactory, gd


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
    env = build_test_env(test_name, "tests/gdy/partial_observability_0_1.yaml", global_observer_type="VECTOR", player_observer_type=gd.ObserverType.VECTOR)

    obs, reward, done, info = env.step([0,0])
    player1_obs = obs[0]
    player2_obs = obs[1]

    assert player1_obs.shape == (1, 3, 3)
    assert player2_obs.shape == (1, 3, 3)



def test_partial_observability_0_2(test_name):
    """
    Using named observers in 0.2 version of GDY
    """
    env = build_test_env(test_name, "tests/gdy/partial_observability_0_2.yaml", global_observer_type="TestVectorObserver", player_observer_type=["TestASCIIObserver", "TestVectorObserver"])

    obs, reward, done, info = env.step([0, 0])
    player1_obs = obs[0]
    player2_obs = obs[1]

    assert player1_obs.shape == (4, 3, 3)
    assert player2_obs.shape == (4, 4, 4)