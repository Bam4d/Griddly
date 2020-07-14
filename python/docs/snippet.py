import gym
import numpy as np
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        "ExampleEnv",
        'RTS/BWDistantResources32x32.yaml',
        level=0
    )

    env = gym.make('GDY-ExampleEnv-v0')
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):
        for p in range(env.action_space.player_count):
            sampled_action_def = np.random.choice(env.action_space.action_names)
            sampled_action_space = env.action_space.action_space_dict[sampled_action_def].sample()

            action = {
                'player': p,
                sampled_action_def: sampled_action_space
            }
            obs, reward, done, info = env.step(action)

            env.render(observer=p)

        env.render(observer='global')