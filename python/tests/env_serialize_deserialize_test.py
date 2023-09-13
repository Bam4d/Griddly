from griddly import gd
from griddly.gym import GymWrapper


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


def test_delayed_action_serialize_deserialize():
    original_env = GymWrapper(
        yaml_file="tests/gdy/delayed_actions.yaml",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
    )
    original_env.reset()

    deserialized_envs = []

    state = original_env.get_state()
    for i in range(4):
        deserialized_envs.append(original_env.load_state(state))

        for env in deserialized_envs:
            env.step([0, 0])

        original_env.step([0, 0])
        state = original_env.get_state()

        for env in deserialized_envs:
            deserialized_state = env.get_state()
            assert state["Hash"] == deserialized_state["Hash"]
