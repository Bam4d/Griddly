import gym
import griddly
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':

    # wrapper = GymWrapperFactory()
    # wrapper.build_gym_from_yaml("Griddly", "nmmo.yaml",
    #                             level=0,player_observer_type=gd.ObserverType.VECTOR,
    #                             global_observer_type=gd.ObserverType.ISOMETRIC)
    env = gym.make("GDY-Partially-Observable-Zelda-v0")
    env.reset()


    # Replace with your own control algorithm!
    for s in range(1000):

        available_actions = env.game.get_available_actions(1)
        print(available_actions)

        for action_location, action_names in available_actions.items():
            available_action_ids = env.game.get_available_action_ids(action_location, list(action_names))
            print(available_action_ids)

        obs, reward, done, info = env.step(env.action_space.sample())
        env.render() # Renders the environment from the perspective of a single player

        env.render(observer='global') # Renders the entire environment