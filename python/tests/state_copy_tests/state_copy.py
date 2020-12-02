import gym
from griddly import gd

if __name__ == '__main__':

    original_env = gym.make(f'GDY-Spiders-v0', level=4, global_observer_type=gd.ObserverType.SPRITE_2D, player_observer_type=gd.ObserverType.SPRITE_2D)
    original_env.enable_history(True)
    original_env.reset()

    env_clones = []

    # make 100 copies of the environment
    for e in range(10):
        env_clones.append(original_env.clone())

    # step them 100 times with the same actions
    for s in range(1000):

        for env in env_clones:
            obs, reward, done, info = env.step(env.action_space.sample())

    for env in env_clones:
        player_observation = env.render(mode='rgb_array')
        global_observation = env.render(mode='rgb_array')
