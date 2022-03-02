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
        global_observer_type=gd.ObserverType.NONE,
        player_observer_type=gd.ObserverType.NONE,
        **kwargs,
    )

    env = gym.make(f"GDY-{test_name}-v0")
    env.reset()
    return env


def test_entity_observations(test_name):
    env = build_test_env(test_name, "tests/gdy/test_entity_observer.yaml")

    current_g_state = env.get_state()
    global_variables = env.game.get_global_variable_names()
    object_variable_map = env.game.get_object_variable_map()

    assert global_variables == ["_steps", "test_global_variable"]
    assert object_variable_map["entity_1"] == ["entity_1_variable"]
    assert object_variable_map["entity_2"] == ["entity_2_variable"]

    entity_observer = env.game.get_entity_observer()

    entity_observation = entity_observer.observe(1)
    entities = entity_observation["Entities"]
    entity_ids = entity_observation["EntityIds"]
    entity_masks = entity_observation["EntityMasks"]

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
