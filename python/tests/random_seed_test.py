import numpy as np

from griddly import gd
from griddly.gym import GymWrapper
from griddly.wrappers import RenderWrapper


def create_env(seed):
    env = GymWrapper(
        yaml_file="Single-Player/Mini-Grid/minigrid-spiders.yaml",
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
        level=3,
    )

    env.reset(seed=seed)
    return env


def test_random_seed_consistency():
    env1 = create_env(1234)
    env2 = create_env(1234)

    global_obs_1_renderer = RenderWrapper(env1, "global", "rgb_array")
    global_obs_2_renderer = RenderWrapper(env1, "global", "rgb_array")

    for i in range(100):
        action1 = env1.action_space.sample()
        action2 = env2.action_space.sample()

        assert action1 == action2

        obs1, reward1, done1, truncated1, info1 = env1.step(action1)
        obs2, reward2, done2, truncated2, info2 = env2.step(action2)

        global_obs1 = global_obs_1_renderer.render()
        global_obs2 = global_obs_2_renderer.render()

        assert np.all(
            global_obs1 == global_obs2
        ), f"global obs differ at test 1 step {i}"

        assert np.all(obs1 == obs2)
        assert reward1 == reward2
        assert done1 == done2
        assert truncated1 == truncated2
        assert info1 == info2

        if done1:
            env1.reset(seed=12345)
            env2.reset(seed=12345)

    for i in range(100):
        action1 = env1.action_space.sample()
        action2 = env2.action_space.sample()

        assert action1 == action2

        obs1, reward1, done1, truncated1, info1 = env1.step(action1)
        obs2, reward2, done2, truncated2, info2 = env2.step(action2)

        global_obs1 = global_obs_1_renderer.render()
        global_obs2 = global_obs_2_renderer.render()

        assert np.all(
            global_obs1 == global_obs2
        ), f" global obs differ at test 2 step {i}"

        assert np.all(obs1 == obs2)
        assert reward1 == reward2
        assert done1 == done2
        assert truncated1 == truncated2
        assert info1 == info2

        if done1:
            env1.reset()
            env2.reset()


def test_random_seed_consistency_after_reset():
    env1 = create_env(None)
    env2 = create_env(None)

    global_obs_1_renderer = RenderWrapper(env1, "global", "rgb_array")
    global_obs_2_renderer = RenderWrapper(env1, "global", "rgb_array")

    for i in range(10):
        action1 = env1.action_space.sample()
        action2 = env2.action_space.sample()

        assert action1 == action2

        obs1, reward1, done1, truncated1, info1 = env1.step(action1)
        obs2, reward2, done2, truncated2, info2 = env2.step(action2)

        global_obs1 = global_obs_1_renderer.render()
        global_obs2 = global_obs_2_renderer.render()

        assert np.all(
            global_obs1 == global_obs2
        ), f"global obs differ at test 1 step {i}"

        assert np.all(obs1 == obs2)
        assert reward1 == reward2
        assert truncated1 == truncated2
        assert done1 == done2
        assert info1 == info2

    env1.reset()
    env2.reset()

    for i in range(10):
        action1 = env1.action_space.sample()
        action2 = env2.action_space.sample()

        assert action1 == action2

        obs1, reward1, done1, truncated1, info1 = env1.step(action1)
        obs2, reward2, done2, truncated2, info2 = env2.step(action2)

        global_obs1 = global_obs_1_renderer.render()
        global_obs2 = global_obs_2_renderer.render()

        assert np.all(
            global_obs1 == global_obs2
        ), f" global obs differ at test 2 step {i}"

        assert np.all(obs1 == obs2)
        assert reward1 == reward2
        assert done1 == done2
        assert truncated1 == truncated2
        assert info1 == info2
