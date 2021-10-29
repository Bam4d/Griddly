import gym
import numpy as np
import pytest

from griddly import gd
from griddly.util.state_hash import StateHasher


@pytest.fixture
def test_name(request):
    return request.node.name


def test_action_and_object_names(test_name):
    for i in range(100):
        env = gym.make('GDY-Sokoban-v0', global_observer_type=gd.ObserverType.VECTOR, player_observer_type=gd.ObserverType.VECTOR)
        env.reset()
        clone_env = env.clone()

        object_names = env.game.get_object_names()
        cloned_object_names = clone_env.game.get_object_names()

        assert object_names == cloned_object_names

def test_available_actions(test_name):

    env = gym.make('GDY-Sokoban-v0', global_observer_type=gd.ObserverType.VECTOR, player_observer_type=gd.ObserverType.VECTOR)
    env.reset()
    clone_env = env.clone()

    available_actions = env.game.get_available_actions(1)
    player_pos = list(available_actions)[0]
    actions = env.game.get_available_action_ids(player_pos, list(available_actions[player_pos]))

    clone_available_actions = clone_env.game.get_available_actions(1)
    clone_player_pos = list(clone_available_actions)[0]
    clone_actions = clone_env.game.get_available_action_ids(clone_player_pos, list(clone_available_actions[clone_player_pos]))

    assert actions == clone_actions

def test_random_trajectory_states(test_name):

    env = gym.make('GDY-Sokoban-v0', global_observer_type=gd.ObserverType.VECTOR, player_observer_type=gd.ObserverType.VECTOR)
    env.reset()
    clone_env = env.clone()

    # Create a bunch of steps and test in both environments
    actions = [env.action_space.sample() for _ in range(100)]

    for action in actions:
        obs, reward, done, info = env.step(action)
        c_obs, c_reward, c_done, c_info = clone_env.step(action)

        assert reward == c_reward
        assert done == c_done
        assert info == c_info

        env_state = env.get_state()
        cloned_state = clone_env.get_state()

        env_state_hasher = StateHasher(env_state)
        cloned_state_hasher = StateHasher(cloned_state)

        env_state_hash = env_state_hasher.hash()
        cloned_state_hash = cloned_state_hasher.hash()

        assert env_state_hash == cloned_state_hash

        if done and c_done:
            env.reset()
            clone_env.reset()

def test_vector_observer(test_name):
    env = gym.make('GDY-Sokoban-v0', global_observer_type=gd.ObserverType.VECTOR, player_observer_type=gd.ObserverType.VECTOR)
    env.reset()
    clone_env = env.clone()

    obs, reward, done, info = env.step(0)
    c_obs, c_reward, c_done, c_info = clone_env.step(0)

    assert np.all(obs == c_obs)


# def test_block_observer(test_name):
#     env = gym.make('GDY-Sokoban-v0', global_observer_type=gd.ObserverType.BLOCK_2D, player_observer_type=gd.ObserverType.BLOCK_2D)
#     env.reset()
#     clone_env = env.clone()
#
#     obs, reward, done, info = env.step(0)
#     c_obs, c_reward, c_done, c_info = clone_env.step(0)
#
#     assert np.all(obs == c_obs)
#
# def test_sprite_observer(test_name):
#     env = gym.make('GDY-Sokoban-v0', global_observer_type=gd.ObserverType.SPRITE_2D, player_observer_type=gd.ObserverType.SPRITE_2D)
#     env.reset()
#     clone_env = env.clone()
#
#     obs, reward, done, info = env.step(0)
#     c_obs, c_reward, c_done, c_info = clone_env.step(0)
#
#     assert np.all(obs == c_obs)