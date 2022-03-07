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

    global_variables = env.game.get_global_variable_names()
    object_variable_map = env.game.get_object_variable_map()

    assert global_variables == ["_steps", "test_global_variable"]
    assert object_variable_map["entity_1"] == ["entity_1_variable"]
    assert object_variable_map["entity_2"] == ["entity_2_variable"]

    obs, reward, done, info = env.step(0)
    entities = obs["Entities"]
    entity_ids = obs["EntityIds"]
    entity_masks = obs["EntityMasks"]

    entity_mask_one = entity_masks["move_one"]
    actors_one = entity_mask_one["ActorEntityIds"]
    mask_one = entity_mask_one["Masks"]

    assert actors_one == [entity_ids["entity_1"][0]]
    assert mask_one == [[1, 1, 1, 1, 0]]

    entity_mask_two = entity_masks["move_two"]
    actors_two = entity_mask_two["ActorEntityIds"]
    mask_two = entity_mask_two["Masks"]

    assert actors_two == [entity_ids["entity_1"][0]]
    assert mask_two == [[1, 1, 1, 1]]


def test_entity_observations_multi_agent(test_name):
    env = build_test_env(test_name, "tests/gdy/test_entity_observer_multi_agent.yaml", global_observer_type=gd.ObserverType.NONE,
        player_observer_type=["EntityObserverOne", "EntityObserverTwo"])

    global_variables = env.game.get_global_variable_names()
    object_variable_map = env.game.get_object_variable_map()

    assert global_variables == ["_steps", "test_global_variable"]
    assert object_variable_map["entity_1"] == ["entity_1_variable"]
    assert object_variable_map["entity_2"] == ["entity_2_variable"]

    obs, reward, done, info = env.step([0, 0])

    player_1_obs = obs[0]

    player_1_entities = player_1_obs["Entities"]
    player_1_entity_ids = player_1_obs["EntityIds"]
    player_1_entity_masks = player_1_obs["EntityMasks"]

    player_1_entity_mask_one = player_1_entity_masks["move_one"]
    player_1_actors_one = player_1_entity_mask_one["ActorEntityIds"]
    player_1_mask_one = player_1_entity_mask_one["Masks"]

    player_2_obs = obs[1]

    player_2_entities = player_2_obs["Entities"]
    player_2_entity_ids = player_2_obs["EntityIds"]
    player_2_entity_masks = player_2_obs["EntityMasks"]

    player_2_entity_mask_one = player_2_entity_masks["move_one"]
    player_2_actors_one = player_2_entity_mask_one["ActorEntityIds"]
    player_2_mask_one = player_2_entity_mask_one["Masks"]
