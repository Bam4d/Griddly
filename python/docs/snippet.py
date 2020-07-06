import gym
import numpy as np
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        "ExampleEnv",
        f'RTS/basicRTS.yaml',
        level=0
    )

    env = gym.make(f'GDY-ExampleEnv-v0')
    player_count = env.player_count
    defined_actions_count = env.defined_actions_count
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):
        for p in range(player_count):
            action_id = env.action_space.sample()
            action_definition_id = np.random.randint(env.defined_actions_count)
            obs, reward, done, info = env.step([p, action_definition_id, *action_id])

            env.render(observer=p)

        env.render(observer='global')