from griddly import GymWrapper, gd
import numpy as np

def create_env():
    env = GymWrapper(
        yaml_file='Single-Player/Mini-Grid/minigrid-spiders.yaml',
        global_observer_type=gd.ObserverType.BLOCK_2D,
        player_observer_type=gd.ObserverType. BLOCK_2D
    )

    env.reset()
    return env

def test_random_seed_consistency():
    env1 = create_env()
    env2 = create_env()
    env1.seed(1234)
    env2.seed(1234)

    for i in range(100):
        action1 = env1.action_space.sample()
        action2 = env2.action_space.sample()

        assert action1 == action2

        obs1, reward1, done1, info1 = env1.step(action1)
        obs2, reward2, done2, info2 = env2.step(action2)

        global_obs1 = env1.render(observer='global', mode='rgb_array')
        global_obs2 = env2.render(observer='global', mode='rgb_array')

        assert np.all(global_obs1 == global_obs2), f'global obs differ at test 1 step {i}'

        assert np.all(obs1 == obs2)
        assert reward1 == reward2
        assert done1 == done2
        assert info1 == info2

        if done1:
            env1.reset()
            env2.reset()

    env1.seed(12345)
    env2.seed(12345)

    for i in range(100):
        action1 = env1.action_space.sample()
        action2 = env2.action_space.sample()

        assert action1 == action2

        obs1, reward1, done1, info1 = env1.step(action1)
        obs2, reward2, done2, info2 = env2.step(action2)

        global_obs1 = env1.render(observer='global', mode='rgb_array')
        global_obs2 = env2.render(observer='global', mode='rgb_array')

        assert np.all(global_obs1 == global_obs2), f' global obs differ at test 2 step {i}'

        assert np.all(obs1 == obs2)
        assert reward1 == reward2
        assert done1 == done2
        assert info1 == info2

        if done1:
            env1.reset()
            env2.reset()

