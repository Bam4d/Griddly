import numpy as np
from griddly import GymWrapperFactory

def test_step_SinglePlayer_SingleActionType_SingleValue():
    """
    Assuming there is a single avatar
    Action is in form env.step(actionId)
    """
    wrapper_factory = GymWrapperFactory()

    pass


def test_step_SinglePlayer_SingleActionType_ArrayValue():
    """
    There is an avatar
    Action is in form env.step([actionId])
    """
    pass


def test_step_SinglePlayer_SelectSource_SingleActionType_ArrayValue():
    """
    There is no avatar

    env.step([x, y, actionId])
    """
    pass


def test_step_SinglePlayer_SelectSource_SingleActionType_MultipleAction():
    """
    There is no avatar
    Player performing multiple actions in a single step

    env.step([
        [x1, y1, actionId1],
        [x2, y2, actionId2]
    ])
    """
    pass


def test_step_MultiplePlayer_SingleActionType_SingleValue():
    """
    There is an avatar
    Multiple players

    env.step([
        [actionId_player1],
        [actionId_player2]
    ])
    """
    pass


def test_step_MultiplePlayer_SingleActionType_ArrayValue():
    """
    There no avatar, multiple players

    env.step([
        [x1, y1, actionId1],
        [x2, y2, actionId2]
    ])
    """
    pass


def test_step_SinglePlayer_MultipleActionType_ArrayValue():
    """
    There is an avatar
    Action is in form env.step([action_type, actionId])
    """
    pass


def test_step_SinglePlayer_SelectSource_MultipleActionType_ArrayValue():
    """
    There is no avatar

    env.step([x, y, action_type, actionId])
    """
    pass


def test_step_SinglePlayer_SelectSource_MultipleActionType_MultipleAction():
    """
    There is no avatar
    Player performing multiple actions in a single step

    env.step([
        [x1, y1, action_type, actionId1],
        [x2, y2, action_type, actionId2]
    ])
    """
    pass


def test_step_MultiplePlayer_MultipleActionType_SingleValue():
    """
    There is an avatar
    Multiple players

    env.step([
        [actionId_player1],
        [actionId_player2]
    ])
    """
    pass


def test_step_MultiplePlayer_MultipleActionType_ArrayValue():
    """
    There no avatar, multiple players

    env.step([
        [x1, y1, action_type, actionId1],
        [x2, y2, action_type, actionId2]
    ])
    """
    pass
