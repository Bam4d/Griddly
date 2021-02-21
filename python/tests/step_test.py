import numpy as np
import gym
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

    env = gym.make(f'GDY-{test_name}-v0')
    env.reset()
    return env


def get_object_state(env, object_name, player=1):
    state = env.get_state()
    for object in state['Objects']:
        if object['Name'] == object_name and object['PlayerId'] == player:
            return object


def test_step_SinglePlayer_SingleActionType_SingleValue(test_name):
    """
    Assuming there is a single avatar
    Action is in form env.step(actionId)
    """

    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == ()
    assert env.action_space.n == 5

    assert env.gdy.get_object_names() == ['avatar']

    obs, reward, done, info = env.step(1)
    avatar_state = get_object_state(env, 'avatar')

    assert avatar_state['Location'] == [1, 3]

    sample = env.action_space.sample()
    assert isinstance(sample, int)


def test_step_SinglePlayer_SingleActionType_ArrayValue(test_name):
    """
    There is an avatar
    Action is in form env.step([actionId])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SingleActionType.yaml"
    )

    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == ()
    assert env.action_space.n == 5

    assert env.gdy.get_object_names() == ['avatar']

    obs, reward, done, info = env.step([1])

    assert obs.shape == (1, 5, 6)
    assert reward == 0
    assert not done
    assert info == {}

    avatar_state = get_object_state(env, 'avatar')

    assert avatar_state['Location'] == [1, 3]

    sample = env.action_space.sample()
    assert isinstance(sample, int)


def test_step_SinglePlayer_SelectSource_SingleActionType(test_name):
    """
    There is no avatar

    env.step([x, y, actionId])
    """

    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SelectSource_SingleActionType.yaml"
    )

    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == (3,)
    assert np.all(env.action_space.nvec == [5, 6, 5])

    assert env.gdy.get_object_names() == ['avatar']

    obs, reward, done, info = env.step([2, 3, 1])

    assert obs.shape == (1, 5, 6)
    assert reward == 0
    assert not done
    assert info == {}

    avatar_state = get_object_state(env, 'avatar')

    assert avatar_state['Location'] == [1, 3]

    sample = env.action_space.sample()
    assert sample.shape == (3,)


def test_step_SinglePlayer_SelectSource_SingleActionType_MultipleAction(test_name):
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
        "tests/gdy/test_step_SinglePlayer_SelectSource_SingleActionType_MultipleAction.yaml"
    )

    assert env.observation_space.shape == (2, 5, 6)
    assert env.global_observation_space.shape == (2, 5, 6)
    assert env.action_space.shape == (3,)
    assert np.all(env.action_space.nvec == [5, 6, 5])

    assert env.gdy.get_object_names() == ['avatar1', 'avatar2']

    obs, reward, done, info = env.step([
        [2, 3, 1],
        [1, 4, 3],
    ])

    assert obs.shape == (2, 5, 6)
    assert reward == 0
    assert not done
    assert info == {}

    avatar1_state = get_object_state(env, 'avatar1')
    avatar2_state = get_object_state(env, 'avatar2')

    assert avatar1_state['Location'] == [1, 3]
    assert avatar2_state['Location'] == [2, 4]

    sample = env.action_space.sample()
    assert sample.shape == (3,)


def test_step_SinglePlayer_MultipleActionType(test_name):
    """
    There is an avatar
    Action is in form env.step([action_type, actionId])
    """

    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_MultipleActionType.yaml"
    )

    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == (2,)
    assert np.all(env.action_space.nvec == [2, 5])

    assert env.gdy.get_object_names() == ['avatar']

    obs, reward, done, info = env.step([0, 1])

    assert obs.shape == (1, 5, 6)
    assert reward == 0
    assert not done
    assert info == {}

    avatar_state = get_object_state(env, 'avatar')
    assert avatar_state['Location'] == [1, 3]

    obs, reward, done, info = env.step([1, 3])

    assert obs.shape == (1, 5, 6)
    assert reward == 1
    assert not done
    assert info == {}

    avatar_state = get_object_state(env, 'avatar')
    assert avatar_state['Location'] == [2, 3]

    sample = env.action_space.sample()
    assert sample.shape == (2,)


def test_step_SinglePlayer_SelectSource_MultipleActionType(test_name):
    """
    There is no avatar

    env.step([x, y, action_type, actionId])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SelectSource_MultipleActionType.yaml"
    )

    assert env.observation_space.shape == (1, 5, 6)
    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.action_space.shape == (4,)
    assert np.all(env.action_space.nvec == [5, 6, 2, 5])

    assert env.gdy.get_object_names() == ['avatar']

    obs, reward, done, info = env.step([2, 3, 0, 1])

    assert obs.shape == (1, 5, 6)
    assert reward == 0
    assert not done
    assert info == {}

    avatar_state = get_object_state(env, 'avatar')
    assert avatar_state['Location'] == [1, 3]

    obs, reward, done, info = env.step([1, 3, 1, 3])

    assert obs.shape == (1, 5, 6)
    assert reward == 1
    assert not done
    assert info == {}

    avatar_state = get_object_state(env, 'avatar')
    assert avatar_state['Location'] == [2, 3]

    sample = env.action_space.sample()
    assert sample.shape == (4,)


def test_step_SinglePlayer_SelectSource_MultipleActionType_MultipleAction(test_name):
    """
    There is no avatar
    Player performing multiple actions in a single step

    env.step([
        [x1, y1, action_type, actionId1],
        [x2, y2, action_type, actionId2]
    ])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_SinglePlayer_SelectSource_MultipleActionType_MultipleAction.yaml"
    )

    assert env.observation_space.shape == (2, 5, 6)
    assert env.global_observation_space.shape == (2, 5, 6)
    assert env.action_space.shape == (4,)
    assert np.all(env.action_space.nvec == [5, 6, 2, 5])

    assert env.gdy.get_object_names() == ['avatar1', 'avatar2']

    obs, reward, done, info = env.step([
        [2, 3, 0, 1],
        [1, 4, 0, 1]
    ])

    assert obs.shape == (2, 5, 6)
    assert reward == 0
    assert not done
    assert info == {}
    
    avatar1_state = get_object_state(env, 'avatar1')
    avatar2_state = get_object_state(env, 'avatar2')
    assert avatar1_state['Location'] == [1, 3]
    assert avatar2_state['Location'] == [0, 4]

    obs, reward, done, info = env.step([
        [1, 3, 1, 3],
        [0, 4, 1, 3]
    ])

    assert obs.shape == (2, 5, 6)
    assert reward == 2
    assert not done
    assert info == {}
    
    avatar1_state = get_object_state(env, 'avatar1')
    avatar2_state = get_object_state(env, 'avatar2')
    assert avatar1_state['Location'] == [2, 3]
    assert avatar2_state['Location'] == [1, 4]

    sample = env.action_space.sample()
    assert sample.shape == (4,)


def test_step_MultiplePlayer_SingleActionType_SingleValue(test_name):
    """
    There is an avatar
    Multiple players

    env.step([
        actionId_player1,
        actionId_player2
    ])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.gdy.get_object_names() == ['avatar']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == ()
        assert env.action_space[p].n == 5

    obs, reward, done, info = env.step([
        1,
        3,
    ])

    assert obs[0].shape == (1, 5, 6)
    assert reward[0] == 0
    assert obs[1].shape == (1, 5, 6)
    assert reward[1] == 0

    player1_avatar_state = get_object_state(env, 'avatar', player=1)
    player2_avatar_state = get_object_state(env, 'avatar', player=2)

    assert player1_avatar_state['Location'] == [0, 3]
    assert player2_avatar_state['Location'] == [4, 3]

    sample = env.action_space.sample()
    assert len(sample) == 2


def test_step_MultiplePlayer_SingleActionType_ArrayValue(test_name):
    """
    There no avatar, multiple players

    env.step([
        [actionId1],
        [actionId2]
    ])
    """

    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SingleActionType.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.gdy.get_object_names() == ['avatar']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == ()
        assert env.action_space[p].n == 5

    obs, reward, done, info = env.step([
        [1],
        [3],
    ])

    assert obs[0].shape == (1, 5, 6)
    assert reward[0] == 0
    assert obs[1].shape == (1, 5, 6)
    assert reward[1] == 0
    assert not done
    assert info == {}

    player1_avatar_state = get_object_state(env, 'avatar', player=1)
    player2_avatar_state = get_object_state(env, 'avatar', player=2)

    assert player1_avatar_state['Location'] == [0, 3]
    assert player2_avatar_state['Location'] == [4, 3]

    sample = env.action_space.sample()
    assert len(sample) == 2


def test_step_MultiplePlayer_MultipleActionType(test_name):
    """
    There is an avatar
    Multiple players

    env.step([
        [action_type, actionId_player1],
        [action_type, actionId_player2]
    ])
    """

    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_MultipleActionType.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.gdy.get_object_names() == ['avatar']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == (2,)
        assert np.all(env.action_space[p].nvec == [2, 5])

    obs, reward, done, info = env.step([
        [0, 1],
        [1, 3],
    ])

    assert obs[0].shape == (1, 5, 6)
    assert reward[0] == 0
    assert obs[1].shape == (1, 5, 6)
    assert reward[1] == 1
    assert not done
    assert info == {}

    player1_avatar_state = get_object_state(env, 'avatar', player=1)
    player2_avatar_state = get_object_state(env, 'avatar', player=2)

    assert player1_avatar_state['Location'] == [0, 3]
    assert player2_avatar_state['Location'] == [4, 3]

    sample = env.action_space.sample()
    assert len(sample) == 2
    assert sample[0].shape == (2,)
    assert sample[1].shape == (2,)


def test_step_MultiplePlayer_SelectSource_SingleActionType(test_name):
    """
        There no avatar, multiple players, single action type

        env.step([
            [x1, y1, actionId1],
            [x2, y2, actionId2]
        ])
        """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SelectSource_SingleActionType.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.gdy.get_object_names() == ['avatar']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == (3,)
        assert np.all(env.action_space[p].nvec == [5, 6, 5])

    obs, reward, done, info = env.step([
        [1, 3, 1],
        [3, 3, 3],
    ])

    assert obs[0].shape == (1, 5, 6)
    assert reward[0] == 0
    assert obs[1].shape == (1, 5, 6)
    assert reward[1] == 0
    assert not done
    assert info == {}

    player1_avatar_state = get_object_state(env, 'avatar', player=1)
    player2_avatar_state = get_object_state(env, 'avatar', player=2)

    assert player1_avatar_state['Location'] == [0, 3]
    assert player2_avatar_state['Location'] == [4, 3]

    sample = env.action_space.sample()
    assert len(sample) == 2
    assert sample[0].shape == (3,)
    assert sample[1].shape == (3,)

def test_step_MultiplePlayer_SelectSource_MultipleActionType(test_name):
    """
    There no avatar, multiple players

    env.step([
        [x1, y1, action_type, actionId1],
        [x2, y2, action_type, actionId2]
    ])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SelectSource_MultipleActionType.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (1, 5, 6)
    assert env.gdy.get_object_names() == ['avatar']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (1, 5, 6)
        assert env.action_space[p].shape == (4,)
        assert np.all(env.action_space[p].nvec == [5, 6, 2, 5])

    obs, reward, done, info = env.step([
        [1, 3, 0, 1],
        [3, 3, 1, 3],
    ])

    assert obs[0].shape == (1, 5, 6)
    assert reward[0] == 0
    assert obs[1].shape == (1, 5, 6)
    assert reward[1] == 1
    assert not done
    assert info == {}

    player1_avatar_state = get_object_state(env, 'avatar', player=1)
    player2_avatar_state = get_object_state(env, 'avatar', player=2)

    assert player1_avatar_state['Location'] == [0, 3]
    assert player2_avatar_state['Location'] == [4, 3]

    sample = env.action_space.sample()
    assert len(sample) == 2
    assert sample[0].shape == (4,)
    assert sample[1].shape == (4,)


def test_step_MultiplePlayer_SelectSource_SingleActionType_MultipleAction(test_name):
    """
    There no avatar, multiple players

    env.step([
        [   # player 1 multiple actions
            [x1, y1, actionId1],
            [x2, y2, actionId2]
        ],
        [   # player 2 multiple actions
            [x1, y1, actionId1],
        ],
    ])
    """
    env = build_test_env(
        test_name,
        "tests/gdy/test_step_MultiPlayer_SelectSource_SingleActionType_MultipleAction.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (2, 5, 6)
    assert env.gdy.get_object_names() == ['avatar1', 'avatar2']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (2, 5, 6)
        assert env.action_space[p].shape == (3,)
        assert np.all(env.action_space[p].nvec == [5, 6, 5])

    obs, reward, done, info = env.step([
        [
            [1, 3, 1],
            [3, 4, 3],
        ],
        [
            [3, 3, 1],
        ]
    ])

    assert obs[0].shape == (2, 5, 6)
    assert reward[0] == 0
    assert obs[1].shape == (2, 5, 6)
    assert reward[1] == 0
    assert not done
    assert info == {}

    player1_avatar1_state = get_object_state(env, 'avatar1', player=1)
    player1_avatar2_state = get_object_state(env, 'avatar2', player=1)

    assert player1_avatar1_state['Location'] == [0, 3]
    assert player1_avatar2_state['Location'] == [4, 4]

    player2_avatar1_state = get_object_state(env, 'avatar1', player=2)
    player2_avatar2_state = get_object_state(env, 'avatar2', player=2)

    assert player2_avatar1_state['Location'] == [2, 3]
    assert player2_avatar2_state['Location'] == [1, 4]

    sample = env.action_space.sample()
    assert len(sample) == 2
    assert sample[0].shape == (3,)
    assert sample[1].shape == (3,)


def test_step_MultiplePlayer_SelectSource_MultipleActionType_MultipleAction(test_name):
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
        "tests/gdy/test_step_MultiPlayer_SelectSource_MultipleActionType_MultipleAction.yaml"
    )

    assert len(env.observation_space) == 2
    assert len(env.action_space) == 2

    assert env.global_observation_space.shape == (2, 5, 6)
    assert env.gdy.get_object_names() == ['avatar1', 'avatar2']

    for p in range(env.player_count):
        assert env.observation_space[p].shape == (2, 5, 6)
        assert env.action_space[p].shape == (4,)
        assert np.all(env.action_space[p].nvec == [5, 6, 2, 5])

    obs, reward, done, info = env.step([
        [
            [1, 3, 0, 1],
            [3, 4, 1, 3],
        ],
        [
            [3, 3, 0, 1],
        ]
    ])

    assert obs[0].shape == (2, 5, 6)
    assert reward[0] == 1
    assert obs[1].shape == (2, 5, 6)
    assert reward[1] == 0
    assert not done
    assert info == {}

    player1_avatar1_state = get_object_state(env, 'avatar1', player=1)
    player1_avatar2_state = get_object_state(env, 'avatar2', player=1)

    assert player1_avatar1_state['Location'] == [0, 3]
    assert player1_avatar2_state['Location'] == [4, 4]

    player2_avatar1_state = get_object_state(env, 'avatar1', player=2)
    player2_avatar2_state = get_object_state(env, 'avatar2', player=2)

    assert player2_avatar1_state['Location'] == [2, 3]
    assert player2_avatar2_state['Location'] == [1, 4]

    sample = env.action_space.sample()
    assert len(sample) == 2
    assert sample[0].shape == (4,)
    assert sample[1].shape == (4,)
