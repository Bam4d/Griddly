import numpy as np
import gymnasium as gym
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

    env = gym.make(f"GDY-{test_name}-v0")
    env.reset()
    return env


def test_CAT_depth_1(test_name):

    env = build_test_env(test_name, "tests/gdy/test_CAT_depth_1.yaml")

    valid_action_trees = env.game.build_valid_action_trees()

    assert len(valid_action_trees) == 1
    assert set(valid_action_trees[0].keys()) == {0, 1, 2, 3}


def test_CAT_depth_2(test_name):

    env = build_test_env(test_name, "tests/gdy/test_CAT_depth_2.yaml")

    valid_action_trees = env.game.build_valid_action_trees()

    assert len(valid_action_trees) == 1
    assert set(valid_action_trees[0].keys()) == {0, 1}

    assert set(valid_action_trees[0][0].keys()) == {0, 1, 2, 3}
    assert set(valid_action_trees[0][1].keys()) == {0, 4}


def test_CAT_depth_3(test_name):

    env = build_test_env(test_name, "tests/gdy/test_CAT_depth_3.yaml")

    valid_action_trees = env.game.build_valid_action_trees()

    assert len(valid_action_trees) == 1
    assert set(valid_action_trees[0].keys()) == {1}
    assert set(valid_action_trees[0][1].keys()) == {1}
    assert set(valid_action_trees[0][1][1].keys()) == {0, 1, 2, 3}


def test_CAT_depth_4(test_name):

    env = build_test_env(test_name, "tests/gdy/test_CAT_depth_4.yaml")

    valid_action_trees = env.game.build_valid_action_trees()

    assert len(valid_action_trees) == 1
    assert set(valid_action_trees[0].keys()) == {1}
    assert set(valid_action_trees[0][1].keys()) == {1}
    assert set(valid_action_trees[0][1][1].keys()) == {0, 1}

    assert set(valid_action_trees[0][1][1][0].keys()) == {0, 1, 2, 3}
    assert set(valid_action_trees[0][1][1][1].keys()) == {0, 4}


def test_CAT_depth_4_2_players(test_name):
    env = build_test_env(test_name, "tests/gdy/test_CAT_depth_4_2_players.yaml")

    valid_action_trees = env.game.build_valid_action_trees()

    assert len(valid_action_trees) == 2
    assert set(valid_action_trees[0].keys()) == {1}
    assert set(valid_action_trees[0][1].keys()) == {1}
    assert set(valid_action_trees[0][1][1].keys()) == {0, 1}

    assert set(valid_action_trees[0][1][1][0].keys()) == {0, 1, 2, 3}
    assert set(valid_action_trees[0][1][1][1].keys()) == {0, 4}

    assert set(valid_action_trees[1].keys()) == {3}
    assert set(valid_action_trees[1][3].keys()) == {1}
    assert set(valid_action_trees[1][3][1].keys()) == {0, 1}

    assert set(valid_action_trees[1][3][1][0].keys()) == {0, 1, 2, 3}
    assert set(valid_action_trees[1][3][1][1].keys()) == {0, 4}
