import numpy as np
import gym
import pytest

from griddly import GymWrapperFactory, gd


@pytest.fixture
def test_name(request):
    return request.node.name


def build_test_env(test_name, level_string):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        'griddly_rts.yaml',
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,

    )

    env = gym.make(f'GDY-{test_name}-v0')
    env.reset(level_string=level_string)
    return env


def test_harvester(test_name):
    """
    Test the mechanics of harvesters
    """

    level_string = (
        'w  w  w  w\n'
        'B1 H1 .  M\n'
        'B2 H2 .  M\n'
        'w  w  w  w\n'
    )

    env = build_test_env(test_name, level_string)

    # Test that the valid action tree con only move right for both
    trees = env.game.build_valid_action_trees()
    assert len(trees) == 2
    # possible x positions
    assert trees[0].keys() == {1}
    assert trees[1].keys() == {1}

    # possible y positions
    assert trees[0][1].keys() == {1}
    assert trees[1][1].keys() == {2}

    # possible action_types (can only move)
    # print(env.action_names)
    assert trees[0][1][1].keys() == {2}
    assert trees[1][1][2].keys() == {2}

    # Both harvesters can only move right (and NOP)
    assert trees[0][1][1][2].keys() == {0, 3}
    assert trees[1][1][2][2].keys() == {0, 3}

    # Move both harvesters right
    observation, reward, done, info = env.step([
        [1, 1, 2, 3],
        [1, 2, 2, 3]
    ])

    assert reward == [0, 0]
    assert done == False

    # Gather right 5 times and check that the harvesters have 5 resources (and check rewards)
    for i in range([1, 2, 3, 4, 5]):
        observation, reward, done, info = env.step([
            [2, 1, 1, 3],
            [2, 2, 1, 3]
        ])

        assert reward == [1, 1]

        # Check that the resource value increases for harvesters
        assert observation[0][11, 2, 1] == i
        assert observation[1][11, 2, 2] == i

        # Check that the resource value decreases for minerals
        assert observation[0][11, 3, 1] == 20 - i
        assert observation[1][11, 3, 2] == 20 - i

    # Check that there are no invalid actions in the tree

    # Check we can no longer harvest
    observation, reward, done, info = env.step([
        [2, 1, 1, 3],
        [2, 2, 1, 3]
    ])

    assert reward == [0, 0]
    assert done == [False, False]