import numpy as np
import gym
import pytest
from griddly import GymWrapperFactory, gd


@pytest.fixture
def test_name(request):
    return request.node.name


def build_test_env(test_name, yaml_file, enable_history=True):
    wrapper_factory = GymWrapperFactory()

    wrapper_factory.build_gym_from_yaml(
        test_name,
        yaml_file,
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )

    env = gym.make(f"GDY-{test_name}-v0")
    env.reset()
    env.enable_history(enable_history)
    return env


def eq_dict(dict1, dict2):

    for key in dict1:
        if key in dict2:
            if dict1[key] != dict2[key]:
                return False
        else:
            assert False
    return True

def in_dict(list_of_dicts, dict):
    for dict1 in list_of_dicts:
        if eq_dict(dict1, dict):
            return True
    return False


def test_history_SinglePlayer_HasHistory(test_name):
    """
    Assuming there is a single avatar
    Action is in form env.step(actionId)
    """

    env = build_test_env(
        test_name, "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    obs, reward, done, info = env.step(1)

    expected_history = [
        {
            "PlayerId": 1,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 1,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [2.0, 3.0],
            "DestinationLocation": [1.0, 3.0],
        }
    ]

    assert info["History"] == expected_history


def test_history_SinglePlayer_NoHistory(test_name):
    """
    Assuming there is a single avatar
    Action is in form env.step(actionId)
    """

    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml",
        enable_history=False,
    )

    obs, reward, done, info = env.step(1)

    assert "History" not in info

    sample = env.action_space.sample()
    assert isinstance(sample, int)


def test_history_SinglePlayer_MultipleAction(test_name):
    """
    There is no avatar
    Player performing multiple actions in a single step

    env.step([
        [x1, y1, actionId1],
        [x2, y2, actionId2]
    ])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SelectSource_SingleActionType_MultipleAction.yaml",
    )

    obs, reward, done, info = env.step([[2, 3, 1], [1, 4, 3],])

    expected_history = [
        {
            "PlayerId": 1,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar1",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 1,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [2.0, 3.0],
            "DestinationLocation": [1.0, 3.0],
        },
        {
            "PlayerId": 1,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar2",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 1,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [1.0, 4.0],
            "DestinationLocation": [2.0, 4.0],
        },
    ]

    assert in_dict(info["History"], expected_history[0])
    assert in_dict(info["History"], expected_history[1])



def test_history_MultiplePlayer_History(test_name):
    """
    There is an avatar
    Multiple players

    env.step([
        [actionId_player1],
        [actionId_player2]
    ])
    """
    env = build_test_env(
        test_name, "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml"
    )

    obs, reward, done, info = env.step([1, 3,])

    expected_history = [
        {
            "PlayerId": 1,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 1,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [1.0, 3.0],
            "DestinationLocation": [0.0, 3.0],
        },
        {
            "PlayerId": 2,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 2,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [3.0, 3.0],
            "DestinationLocation": [4.0, 3.0],
        },
    ]

    assert in_dict(info["History"], expected_history[0])
    assert in_dict(info["History"], expected_history[1])


def test_history_MultiplePlayer_MultipleAction_History(test_name):
    """
    There no avatar, multiple players

    env.step([
        [   # player 1 multiple actions
            [x1, y1, action_type, actionId1],
            [x2, y2, action_type, actionId2]
        ],
        [   # player 2 multiple actions
            [x1, y1, action_type, actionId1],
        ],
    ])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SelectSource_MultipleActionType_MultipleAction.yaml",
    )

    obs, reward, done, info = env.step(
        [[[1, 3, 0, 1], [3, 4, 1, 3],], [[3, 3, 0, 1],],]
    )

    expected_history = [
        {
            "PlayerId": 1,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar1",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 1,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [1.0, 3.0],
            "DestinationLocation": [0.0, 3.0],
        },
        {
            "PlayerId": 1,
            "ActionName": "other_move",
            "Tick": 0,
            "Rewards": {1: 1},
            "Delay": 0,
            "SourceObjectName": "avatar2",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 1,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [3.0, 4.0],
            "DestinationLocation": [4.0, 4.0],
        },
        {
            "PlayerId": 2,
            "ActionName": "move",
            "Tick": 0,
            "Rewards": {},
            "Delay": 0,
            "SourceObjectName": "avatar1",
            "DestinationObjectName": "_empty",
            "SourceObjectPlayerId": 2,
            "DestinationObjectPlayerId": 0,
            "SourceLocation": [3.0, 3.0],
            "DestinationLocation": [2.0, 3.0],
        },
    ]

    assert in_dict(info["History"], expected_history[0])
    assert in_dict(info["History"], expected_history[1])
    assert in_dict(info["History"], expected_history[2])
