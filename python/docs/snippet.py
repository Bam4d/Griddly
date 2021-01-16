import gym
import griddly

if __name__ == '__main__':

    env = gym.make('GDY-Partially-Observable-Zelda-v0')
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):
        obs, reward, done, info = env.step(env.action_space.sample())
        env.render() # Renders the environment from the perspective of a single player

        env.render(observer='global') # Renders the entire environment