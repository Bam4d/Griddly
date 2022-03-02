import numpy as np
import gym
import pytest
from griddly import GymWrapperFactory, gd
from griddly.util.wrappers import ValidActionSpaceWrapper


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
    env = ValidActionSpaceWrapper(env)
    return env


def get_object_state(env, object_name, player=1):
    state = env.get_state()
    for object in state["Objects"]:
        if object["Name"] == object_name and object["PlayerId"] == player:
            return object


def check_valid_actions(sampled_actions, possible_actions):
    for possible in possible_actions:
        if np.all(sampled_actions == possible):
            return True

    return False


def test_vasw_SinglePlayer_SingleActionType(test_name):
    env = build_test_env(
        test_name, "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    assert env.action_space.shape == ()
    assert env.action_space.n == 5

    # Test masks
    unit_location_mask = env.get_unit_location_mask(1)

    expected_location_mask = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask == expected_location_mask)
    action_mask = env.get_unit_action_mask((2, 3), ["move"])

    expected_action_id_mask = [1, 1, 1, 1, 1]

    assert np.all(action_mask["move"] == expected_action_id_mask)

    possible_actions = [[0], [1], [2], [3], [4]]

    # Sample a valid action
    env.action_space.seed(0)
    sampled = env.action_space.sample()

    assert check_valid_actions(sampled, possible_actions)


def test_vasw_SinglePlayer_SelectSource_SingleActionType(test_name):
    env = build_test_env(
        test_name, "tests/gdy/test_step_SinglePlayer_SelectSource_SingleActionType.yaml"
    )

    assert env.action_space.shape == (3,)
    assert np.all(env.action_space.nvec == [5, 6, 5])

    # Test masks
    unit_location_mask = env.get_unit_location_mask(1)

    expected_location_mask = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask == expected_location_mask)
    action_mask = env.get_unit_action_mask((2, 3), ["move"])

    expected_action_id_mask = [1, 1, 1, 1, 1]

    assert np.all(action_mask["move"] == expected_action_id_mask)

    possible_actions = [[2, 3, 0], [2, 3, 1], [2, 3, 2], [2, 3, 3], [2, 3, 4]]

    # Sample a valid action
    sampled = env.action_space.sample()

    assert check_valid_actions(sampled, possible_actions)


def test_vasw_SinglePlayer_MultipleActionType(test_name):
    env = build_test_env(
        test_name, "tests/gdy/test_step_SinglePlayer_MultipleActionType.yaml"
    )

    assert env.action_space.shape == (2,)
    assert np.all(env.action_space.nvec == [2, 5])

    # Test masks
    unit_location_mask = env.get_unit_location_mask(1)

    expected_location_mask = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask == expected_location_mask)
    action_mask1 = env.get_unit_action_mask((2, 3), ["move"])
    expected_action_id_mask1 = [1, 1, 1, 1, 1]
    assert np.all(action_mask1["move"] == expected_action_id_mask1)

    action_mask2 = env.get_unit_action_mask((2, 3), ["other_move"])
    expected_action_id_mask2 = [1, 1, 1, 1, 1]
    assert np.all(action_mask2["other_move"] == expected_action_id_mask2)

    all_action_mask = env.get_unit_action_mask((2, 3), ["move", "other_move"])

    assert np.all(all_action_mask["move"] == expected_action_id_mask1)
    assert np.all(all_action_mask["other_move"] == expected_action_id_mask2)

    possible_actions = [
        [0, 0],
        [1, 0],
        [0, 1],
        [1, 1],
        [0, 2],
        [1, 2],
        [0, 3],
        [1, 3],
        [0, 4],
        [1, 4],
    ]

    # Sample a valid action
    sampled = env.action_space.sample()

    assert check_valid_actions(sampled, possible_actions)


def test_vasw_SinglePlayer_SelectSource_MultipleActionType(test_name):
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SelectSource_MultipleActionType.yaml",
    )

    assert env.action_space.shape == (4,)
    assert np.all(env.action_space.nvec == [5, 6, 2, 5])

    # Test masks
    unit_location_mask = env.get_unit_location_mask(1)

    expected_location_mask = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask == expected_location_mask)
    action_mask1 = env.get_unit_action_mask((2, 3), ["move"])
    expected_action_id_mask1 = [1, 1, 1, 1, 1]
    assert np.all(action_mask1["move"] == expected_action_id_mask1)

    action_mask2 = env.get_unit_action_mask((2, 3), ["other_move"])
    expected_action_id_mask2 = [1, 1, 1, 1, 1]
    assert np.all(action_mask2["other_move"] == expected_action_id_mask2)

    all_action_mask = env.get_unit_action_mask((2, 3), ["move", "other_move"])

    assert np.all(all_action_mask["move"] == expected_action_id_mask1)
    assert np.all(all_action_mask["other_move"] == expected_action_id_mask2)

    possible_actions = [
        [2, 3, 0, 0],
        [2, 3, 1, 0],
        [1, 4, 0, 0],
        [1, 4, 1, 0],
        [2, 3, 0, 1],
        [2, 3, 1, 1],
        [1, 4, 0, 1],
        [1, 4, 1, 1],
        [2, 3, 0, 2],
        [2, 3, 1, 2],
        [1, 4, 0, 2],
        [1, 4, 1, 2],
        [2, 3, 0, 3],
        [2, 3, 1, 3],
        [1, 4, 0, 3],
        [1, 4, 1, 3],
        [2, 3, 0, 4],
        [2, 3, 1, 4],
        [1, 4, 0, 4],
        [1, 4, 1, 4],
    ]

    # Sample a valid action
    sampled = env.action_space.sample()

    assert check_valid_actions(sampled, possible_actions)


def test_vasw_MultiPlayer_SingleActionType(test_name):
    env = build_test_env(
        test_name, "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml"
    )

    assert len(env.action_space) == 2

    for p in range(env.player_count):
        assert env.action_space[p].shape == ()
        assert env.action_space[p].n == 5

    # Test masks
    unit_location_mask1 = env.get_unit_location_mask(1)

    expected_location_mask1 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask1 == expected_location_mask1)

    unit_location_mask2 = env.get_unit_location_mask(2)

    expected_location_mask2 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask2 == expected_location_mask2)

    action_mask = env.get_unit_action_mask((1, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    action_mask = env.get_unit_action_mask((3, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    possible_actions = [[0], [1], [2], [3], [4]]

    # Sample a valid action
    env.action_space.seed(0)
    sampled = env.action_space.sample()

    # Test that we sample both players
    assert check_valid_actions(sampled[0], possible_actions)
    assert check_valid_actions(sampled[1], possible_actions)


def test_vasw_MultiplePlayer_SelectSource_SingleActionType(test_name):
    env = build_test_env(
        test_name, "tests/gdy/test_step_MultiPlayer_SelectSource_SingleActionType.yaml"
    )

    assert len(env.action_space) == 2

    for p in range(env.player_count):
        assert env.action_space[p].shape == (3,)
        assert np.all(env.action_space[p].nvec == [5, 6, 5])

    # Test masks
    unit_location_mask1 = env.get_unit_location_mask(1)

    expected_location_mask1 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask1 == expected_location_mask1)

    unit_location_mask2 = env.get_unit_location_mask(2)

    expected_location_mask2 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask2 == expected_location_mask2)

    action_mask = env.get_unit_action_mask((1, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    action_mask = env.get_unit_action_mask((3, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    possible_actions = [
        [[1, 3, 0], [1, 3, 1], [1, 3, 2], [1, 3, 3], [1, 3, 4]],
        [[3, 3, 0], [3, 3, 1], [3, 3, 2], [3, 3, 3], [3, 3, 4]],
    ]

    # Sample a valid action
    env.action_space.seed(0)
    sampled = env.action_space.sample()

    # Test that we sample both players
    assert check_valid_actions(sampled[0], possible_actions[0])
    assert check_valid_actions(sampled[1], possible_actions[1])


def test_vasw_MultiplePlayer_MultipleActionType(test_name):
    env = build_test_env(
        test_name, "tests/gdy/test_step_MultiPlayer_MultipleActionType.yaml"
    )

    assert len(env.action_space) == 2

    for p in range(env.player_count):
        assert env.action_space[p].shape == (2,)
        assert np.all(env.action_space[p].nvec == [2, 5])

    # Test masks
    unit_location_mask1 = env.get_unit_location_mask(1)

    expected_location_mask1 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask1 == expected_location_mask1)

    unit_location_mask2 = env.get_unit_location_mask(2)

    expected_location_mask2 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask2 == expected_location_mask2)

    action_mask = env.get_unit_action_mask((1, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    action_mask = env.get_unit_action_mask((3, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    possible_actions = [
        [
            [0, 0],
            [1, 0],
            [0, 1],
            [1, 1],
            [0, 2],
            [1, 2],
            [0, 3],
            [1, 3],
            [0, 4],
            [1, 4],
        ],
        [
            [0, 0],
            [1, 0],
            [0, 1],
            [1, 1],
            [0, 2],
            [1, 2],
            [0, 3],
            [1, 3],
            [0, 4],
            [1, 4],
        ],
    ]

    # Sample a valid action
    env.action_space.seed(0)
    sampled = env.action_space.sample()

    # Test that we sample both players
    assert check_valid_actions(sampled[0], possible_actions[0])
    assert check_valid_actions(sampled[1], possible_actions[1])


def test_vasw_MultiplePlayer_SelectSource_MultipleActionType(test_name):
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SelectSource_MultipleActionType.yaml",
    )

    assert len(env.action_space) == 2

    for p in range(env.player_count):
        assert env.action_space[p].shape == (4,)
        assert np.all(env.action_space[p].nvec == [5, 6, 2, 5])

    # Test masks
    unit_location_mask1 = env.get_unit_location_mask(1)

    expected_location_mask1 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask1 == expected_location_mask1)

    unit_location_mask2 = env.get_unit_location_mask(2)

    expected_location_mask2 = [
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 0, 0],
    ]

    assert np.all(unit_location_mask2 == expected_location_mask2)

    action_mask = env.get_unit_action_mask((1, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    action_mask = env.get_unit_action_mask((3, 3), ["move"])
    expected_action_id_mask = [1, 1, 1, 1, 1]
    assert np.all(action_mask["move"] == expected_action_id_mask)

    possible_actions = [
        [
            [1, 3, 0, 0],
            [1, 3, 1, 0],
            [1, 3, 0, 1],
            [1, 3, 1, 1],
            [1, 3, 0, 2],
            [1, 3, 1, 2],
            [1, 3, 0, 3],
            [1, 3, 1, 3],
            [1, 3, 0, 4],
            [1, 3, 1, 4],
        ],
        [
            [3, 3, 0, 0],
            [3, 3, 1, 0],
            [3, 3, 0, 1],
            [3, 3, 1, 1],
            [3, 3, 0, 2],
            [3, 3, 1, 2],
            [3, 3, 0, 3],
            [3, 3, 1, 3],
            [3, 3, 0, 4],
            [3, 3, 1, 4],
        ],
    ]

    # Sample a valid action
    env.action_space.seed(0)
    sampled = env.action_space.sample()

    # Test that we sample both players
    assert check_valid_actions(sampled[0], possible_actions[0])
    assert check_valid_actions(sampled[1], possible_actions[1])
