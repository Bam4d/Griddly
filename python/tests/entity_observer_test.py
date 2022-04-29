import numpy as np
import gym
import pytest
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


def test_entity_observations(test_name):
    env = build_test_env(test_name, "tests/gdy/test_entity_observer.yaml", global_observer_type=gd.ObserverType.NONE,
                         player_observer_type=gd.ObserverType.ENTITY)

    obs, reward, done, info = env.step(0)
    entities = obs["Entities"]
    entity_ids = obs["Ids"]
    entity_locations = obs["Locations"]

    entity_1s = entities["entity_1"]
    entity_1_ids = entity_ids["entity_1"]
    assert len(entity_1s) == 1
    assert len(entity_1_ids) == 1
    assert len(entity_1s[0]) == 3
    assert entity_1s[0][0] == 1
    assert entity_1s[0][1] == 1

    entity_2s = entities["entity_2"]
    entity_2_ids = entity_ids["entity_2"]
    assert len(entity_2s) == 1
    assert len(entity_2_ids) == 1
    assert len(entity_2s[0]) == 3
    assert entity_2s[0][0] == 1
    assert entity_2s[0][1] == 2

    actor_masks = obs["ActorMasks"]
    actor_ids = obs["ActorIds"]

    actor_mask_one = actor_masks["move_one"]
    actor_ids_one = actor_ids["move_one"]

    assert actor_ids_one == [entity_ids["entity_1"][0]]
    assert actor_mask_one == [[1, 1, 1, 1, 0]]

    actor_mask_two = actor_masks["move_two"]
    actor_ids_two = actor_ids["move_two"]

    assert actor_ids_two == [entity_ids["entity_1"][0]]
    assert actor_mask_two == [[1, 1, 1, 1]]


def test_entity_observations_multi_agent(test_name):
    env = build_test_env(test_name, "tests/gdy/test_entity_observer_multi_agent.yaml",
                         global_observer_type=gd.ObserverType.NONE,
                         player_observer_type=["EntityObserverOne", "EntityObserverTwo"])


    player_1_space = env.player_observation_space[0].features
    player_2_space = env.player_observation_space[1].features

    assert player_1_space["entity_1"] == ["x", "y", "z", "playerId", "entity_1_variable"]
    assert player_1_space["entity_2"] == ["x", "y", "z", "ox", "oy", "entity_2_variable"]
    assert player_1_space["__global__"] == ["test_perplayer_variable", "test_global_variable"]

    assert player_2_space["entity_1"] == ["x", "y", "z"]
    assert player_2_space["entity_2"] == ["x", "y", "z"]
    assert player_2_space["__global__"] == ["test_global_variable"]

    obs, reward, done, info = env.step([0, 0])

    player_1_obs = obs[0]

    player_1_entities = player_1_obs["Entities"]
    player_1_entity_ids = player_1_obs["Ids"]

    p1_globals = player_1_entities["__global__"]
    assert np.all(p1_globals[0] == [12.0, 0.0])

    p1_entity_1s = player_1_entities["entity_1"]
    p1_entity_1_ids = player_1_entity_ids["entity_1"]
    assert len(p1_entity_1s) == 2
    assert len(p1_entity_1_ids) == 2
    assert len(p1_entity_1s[0]) == 5
    assert len(p1_entity_1s[1]) == 5

    p1_entity_2s = player_1_entities["entity_2"]
    p1_entity_2_ids = player_1_entity_ids["entity_2"]
    assert len(p1_entity_2s) == 2
    assert len(p1_entity_2_ids) == 2
    assert len(p1_entity_2s[0]) == 6
    assert len(p1_entity_2s[1]) == 6

    player_2_obs = obs[1]

    player_2_entities = player_2_obs["Entities"]
    player_2_entity_ids = player_2_obs["Ids"]

    p2_globals = player_2_entities["__global__"]
    assert np.all(p2_globals[0] == [0])

    p2_entity_1s = player_2_entities["entity_1"]
    p2_entity_1_ids = player_2_entity_ids["entity_1"]
    assert len(p2_entity_1s) == 2
    assert len(p2_entity_1_ids) == 2
    assert len(p2_entity_1s[0]) == 3
    assert len(p2_entity_1s[1]) == 3

    p2_entity_2s = player_2_entities["entity_2"]
    p2_entity_2_ids = player_2_entity_ids["entity_2"]
    assert len(p2_entity_2s) == 2
    assert len(p2_entity_2_ids) == 2
    assert len(p2_entity_2s[0]) == 3
    assert len(p2_entity_2s[1]) == 3
