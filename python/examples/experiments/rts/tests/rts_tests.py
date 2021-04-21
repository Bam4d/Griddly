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

    # possible action_types (can move and attack)
    # print(env.action_names)
    assert trees[0][1][1].keys() == {2, 3}
    assert trees[1][1][2].keys() == {2, 3}

    # Both harvesters can only move right (and NOP)
    assert trees[0][1][1][2].keys() == {0, 3}
    assert trees[1][1][2][2].keys() == {0, 3}

    # Move both harvesters right
    observation, reward, done, info = env.step([
        [1, 1, 2, 3],
        [1, 2, 2, 3]
    ])

    can_gather_tree = env.game.build_valid_action_trees()

    # Can gather, move and attack
    assert can_gather_tree[0][2][1].keys() == {1, 2, 3}
    assert can_gather_tree[1][2][2].keys() == {1, 2, 3}

    assert reward == [0, 0]
    assert done == False

    # Gather right 5 times and check that the harvesters have 5 resources (and check rewards)
    for i in range(1, 6):
        observation, reward, done, info = env.step([
            [2, 1, 1, 3],
            [2, 2, 1, 3]
        ])

        assert reward == [1, 1]

        # Check that the resource value increases for harvesters
        assert observation[0][12, 2, 1] == i
        assert observation[1][12, 2, 2] == i

        # Check that the resource value decreases for minerals
        assert observation[0][12, 3, 1] == 20 - i
        assert observation[1][12, 3, 2] == 20 - i

    # Check that there are no invalid actions in the tree
    no_more_gather_tree = env.game.build_valid_action_trees()

    # Can move and attack
    assert no_more_gather_tree[0][2][1].keys() == {2, 3}
    assert no_more_gather_tree[1][2][2].keys() == {2, 3}

    # Check that gather does nothing
    observation, reward, done, info = env.step([
        [2, 1, 1, 3],
        [2, 2, 1, 3]
    ])

    # Check that the resource is not increased
    assert observation[0][12, 2, 1] == 5
    assert observation[1][12, 2, 2] == 5

    # Check that the resource value is not decreased
    assert observation[0][12, 3, 1] == 15
    assert observation[1][12, 3, 2] == 15

    assert reward == [0, 0]
    assert done == False

    # Move back next to the base
    observation, reward, done, info = env.step([
        [2, 1, 2, 1],
        [2, 2, 2, 1]
    ])

    # Pass all the resources to the base
    for i in [4, 3, 2, 1, 0]:
        observation, reward, done, info = env.step([
            [1, 1, 1, 1],
            [1, 2, 1, 1]
        ])

        assert reward == [1, 1]

        # Check that the resource value increases for harvesters
        assert observation[0][12, 1, 1] == i
        assert observation[1][12, 1, 2] == i

    # cannot give more resources to the base
    observation, reward, done, info = env.step([
        [1, 1, 1, 1],
        [1, 2, 1, 1]
    ])

    assert observation[0][12, 1, 1] == 0
    assert observation[1][12, 1, 2] == 0

    # Now the players have 5 resources each, we should be able to build a new harvester
    can_build_harvester = env.game.build_valid_action_trees()
    assert can_build_harvester[0][0][1][0].keys() == {0, 1}
    assert can_build_harvester[1][0][2][0].keys() == {0, 1}

    assert env.get_state()['GlobalVariables']['player_resources'][1] == 5
    assert env.get_state()['GlobalVariables']['player_resources'][2] == 5

    # Build both harvesters
    observation, reward, done, info = env.step([
        [0, 1, 0, 1],
        [0, 2, 0, 1]
    ])

    assert env.get_state()['GlobalVariables']['player_resources'][1] == 0
    assert env.get_state()['GlobalVariables']['player_resources'][2] == 0

    assert reward == [1, 1]

