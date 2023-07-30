import numpy as np
import pytest
from griddly import gd
from griddly.gym import GymWrapper
import os


@pytest.fixture
def test_name(request):
    return request.node.name


def get_state_for_object(object_name, state):
    for o in state["Objects"]:
        if o["Name"] == object_name:
            return o
    return None


def test_conditionals_preconditions(test_name):
    current_path = os.path.dirname(os.path.realpath(__file__))
    env = GymWrapper(
        current_path + "/conditionals.yaml",
        player_observer_type=gd.ObserverType.VECTOR,
        global_observer_type=gd.ObserverType.VECTOR,
        level=0,
    )
    env.reset()

    object1_location = (5, 5)
    object2_location = (7, 3)
    object3_location = (8, 6)

    object_action_ids = {}

    # firstly check that the actions are available (calculated from preconditions)
    # object2 can move
    # object1 and 2 can use energy
    # object1, object2 and object3 can toggle_thing
    for location, action_names in env.game.get_available_actions(1).items():
        object_action_ids[location] = env.game.get_available_action_ids(list(location), list(action_names))

    object_1_available_action_ids = object_action_ids[object1_location]
    object_2_available_action_ids = object_action_ids[object2_location]
    object_3_available_action_ids = object_action_ids[object3_location]

    # Move
    assert sorted(object_1_available_action_ids["move"]) == sorted([])
    assert sorted(object_2_available_action_ids["move"]) == sorted([1, 2, 3, 4])
    assert sorted(object_3_available_action_ids["move"]) == sorted([])

    # use_energy
    assert sorted(object_1_available_action_ids["use_energy"]) == sorted([1, 2, 3, 4])
    assert sorted(object_2_available_action_ids["use_energy"]) == sorted([1, 2, 3, 4])
    assert sorted(object_3_available_action_ids["use_energy"]) == sorted([])

    # toggle_thing
    assert sorted(object_1_available_action_ids["toggle_thing"]) == sorted([1, 2, 3, 4])
    assert sorted(object_2_available_action_ids["toggle_thing"]) == sorted([1, 2, 3, 4])
    assert sorted(object_3_available_action_ids["toggle_thing"]) == sorted([1, 2, 3, 4])


def test_conditionals_actions(test_name):
    current_path = os.path.dirname(os.path.realpath(__file__))
    env = GymWrapper(
        current_path + "/conditionals.yaml",
        player_observer_type=gd.ObserverType.VECTOR,
        global_observer_type=gd.ObserverType.VECTOR,
        level=0,
    )
    env.reset()

    move_id = env.action_names.index("move")
    use_energy_id = env.action_names.index("use_energy")
    toggle_thing_id = env.action_names.index("toggle_thing")

    # object1
    env.step([5, 5, move_id, 1])  # object1 will not move
    object1_state = get_state_for_object("object1", env.get_state())
    assert object1_state["Location"] == [5, 5]
    env.step([5, 5, use_energy_id, 1])  # object1 will now have 0 energy
    object1_state = get_state_for_object("object1", env.get_state())
    assert object1_state["Variables"]["has_energy"] == 0
    env.step([5, 5, toggle_thing_id, 1])  # object1 will now have 2 "thing"
    object1_state = get_state_for_object("object1", env.get_state())
    assert object1_state["Variables"]["thing"] == 2

    # object2
    env.step([7, 3, move_id, 1])
    object2_state = get_state_for_object("object2", env.get_state())
    assert object2_state["Location"] == [6, 3]
    env.step([6, 3, use_energy_id, 1])
    object2_state = get_state_for_object("object2", env.get_state())
    assert object2_state["Variables"]["has_energy"] == 0
    env.step([6, 3, toggle_thing_id, 1])
    object2_state = get_state_for_object("object2", env.get_state())
    assert object2_state["Variables"]["thing"] == 2

    # object3
    env.step([8, 6, move_id, 1])
    object3_state = get_state_for_object("object3", env.get_state())
    assert object3_state["Location"] == [8, 6]
    env.step([8, 6, use_energy_id, 1])
    object3_state = get_state_for_object("object3", env.get_state())
    assert object3_state["Variables"]["has_energy"] == 0
    env.step([8, 6, toggle_thing_id, 1])
    object3_state = get_state_for_object("object3", env.get_state())
    assert object3_state["Variables"]["thing"] == 0
