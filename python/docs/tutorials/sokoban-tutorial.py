import gym

from griddly_python import GymWrapperFactory
from griddly_python.RenderTools import RenderToFile

if __name__ == '__main__':
    # A nice tool to save png images
    file_renderer = RenderToFile()

    # This is what to use if you want to use OpenAI gym environments
    wrapper = GymWrapperFactory()

    # There are two levels here
    level = 0
    wrapper.build_gym_from_yaml('sokoban', 'tutorials/sokoban.yaml', level=level)

    # Create the Environment
    env = gym.make(f'GDY-sokoban-v0')
    observation = env.reset()

    file_renderer.render(observation, f'sokoban-level-{level}.png')