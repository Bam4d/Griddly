import gym
import pytest
from griddly.RenderTools import VideoRecorder

from griddly import GymWrapperFactory, gd
from griddly.util.environment_generator_generator import EnvironmentGeneratorGenerator
from griddly.util.wrappers import ValidActionSpaceWrapper


@pytest.fixture
def test_name(request):
    return request.node.name

def build_generator(test_name, yaml_file):
    wrapper_factory = GymWrapperFactory()
    egg = EnvironmentGeneratorGenerator(yaml_file=yaml_file)
    generator_yaml = egg.generate_env_yaml((10,10))

    wrapper_factory.build_gym_from_yaml_string(
        test_name,
        yaml_string=generator_yaml,
        # TODO: Change this to ASCII observer when its ready
        global_observer_type=gd.ObserverType.SPRITE_2D,
        player_observer_type=gd.ObserverType.SPRITE_2D,
    )

    env = gym.make(f'GDY-{test_name}-v0')
    env.reset()
    return env

def test_spider_nest_generator(test_name):

    yaml_file = 'Single-Player/GVGAI/spider-nest.yaml'

    genv = build_generator(test_name, yaml_file)

    # visualization = genv.render(observer=0, mode='rgb_array')
    # video_recorder = VideoRecorder()
    # video_recorder.start('generator_video_test.mp4', visualization.shape)

    # Place 10 Random Objects
    for i in range(0,1000):
        action = genv.action_space.sample()
        obs, reward, done, info = genv.step(action)

        state = genv.get_state()

        visual = genv.render(observer=0, mode='rgb_array')
        # video_recorder.add_frame(visual)


    # video_recorder.close()
