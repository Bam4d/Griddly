from griddly import GymWrapper, gd


def test_env_serialize_deserialize():

    env = GymWrapper(
        yaml_file="tests/gdy/multi_agent_remove.yaml",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )
    env.reset()

    state = env.get_state()

    env2 = env.load_state(state)

    state2 = env2.get_state()

    assert state["Hash"] == state2["Hash"]




