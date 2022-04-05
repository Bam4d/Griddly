import numpy as np
import pytest
import json
from griddly import gd, GymWrapper
import os


@pytest.fixture
def test_name(request):
    return request.node.name


def test_conditionals():
    current_path = os.path.dirname(os.path.realpath(__file__))
    env = GymWrapper(
        current_path + "/conditionals.yaml",
        player_observer_type=gd.ObserverType.VECTOR,
        global_observer_type=gd.ObserverType.VECTOR,
        level=0,
    )
    env.reset()





print("complete")
