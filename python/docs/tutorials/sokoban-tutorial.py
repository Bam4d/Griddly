import gym

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import RenderToFile

if __name__ == '__main__':
    # A nice tool to save png images
    file_renderer = RenderToFile()

    # This is what to use if you want to use OpenAI gym environments
    wrapper = GymWrapperFactory()

    # There are two levels here
    level = 1
    wrapper.build_gym_from_yaml('Sokoban', 'tutorials/sokoban.yaml', player_observer_type=gd.ObserverType.BLOCK_2D, level=level)

    # Create the Environment
    env = gym.make(f'GDY-Sokoban-v0')
    observation = env.reset()

    file_renderer.render(observation, f'sokoban-level-{level}.png')