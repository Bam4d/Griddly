import numpy as np
import pytest

import gymnasium as gym
from griddly import gd, GymWrapper


@pytest.fixture
def test_name(request):
    return request.node.name


def test_action_and_object_names(test_name):
    for i in range(20):
        env = gym.make(
            "GDY-Sokoban-v0",
            global_observer_type=gd.ObserverType.VECTOR,
            player_observer_type=gd.ObserverType.VECTOR,
        )
        env.reset()
        clone_env = env.clone()

        object_names = env.game.get_object_names()
        cloned_object_names = clone_env.game.get_object_names()

        assert object_names == cloned_object_names


def test_available_actions(test_name):

    env = gym.make(
        "GDY-Sokoban-v0",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )
    env.reset()
    clone_env = env.clone()

    available_actions = env.game.get_available_actions(1)
    player_pos = list(available_actions)[0]
    actions = env.game.get_available_action_ids(
        player_pos, list(available_actions[player_pos])
    )

    clone_available_actions = clone_env.game.get_available_actions(1)
    clone_player_pos = list(clone_available_actions)[0]
    clone_actions = clone_env.game.get_available_action_ids(
        clone_player_pos, list(clone_available_actions[clone_player_pos])
    )

    assert actions == clone_actions


def test_random_trajectory_states(test_name):

    env = gym.make(
        "GDY-Sokoban-v0",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )
    env.reset()
    clone_env = env.clone()

    # Create a bunch of steps and test in both environments
    actions = [env.action_space.sample() for _ in range(1000)]

    for action in actions:
        obs, reward, done, truncated, info = env.step(action)
        c_obs, c_reward, c_done, c_truncated, c_info = clone_env.step(action)

        assert reward == c_reward
        assert done == c_done
        assert info == c_info

        env_state = env.get_state()
        cloned_state = clone_env.get_state()

        assert (
            env_state["Hash"] == cloned_state["Hash"]
        ), f"state: {env_state}, cloned: {cloned_state}"

        if done and c_done:
            env.reset()
            clone_env.reset()


def test_vector_observer(test_name):
    env = gym.make(
        "GDY-Sokoban-v0",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )
    env.reset()
    clone_env = env.clone()

    obs, reward, done, truncated, info = env.step(0)
    c_obs, c_reward, c_done, c_truncated, c_info = clone_env.step(0)

    assert np.all(obs == c_obs)


def test_clone_multi_agent_done(test_name):
    """
    In multi agent scenario we want to test that if one of the agents in multi-agent game is "done" then we can clone
    the environment without issue
    """
    env = GymWrapper(
        yaml_file="tests/gdy/multi_agent_remove.yaml",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )
    env.reset()

    # Remove one of the agents
    obs_1, reward_1, done_1, truncated_1, info_1 = env.step([0, 1])

    clone_env = env.clone()

    # Remove the other
    obs_2, reward_2, done_2, truncated_2, info_2 = env.step([1, 0])
    c_obs, c_reward, c_done, c_truncated, c_info = clone_env.step([1, 0])

    assert done_2

    assert np.all(np.array(obs_2) == np.array(c_obs))
    assert np.all(reward_2 == c_reward)
    assert np.all(done_2 == c_done)

if __name__ == "__main__":
    test_random_trajectory_states("blah")