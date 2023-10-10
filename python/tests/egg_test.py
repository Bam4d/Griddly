import gymnasium as gym
import pytest

from griddly import gd
from griddly.gym import GymWrapperFactory
from griddly.util.environment_generator_generator import EnvironmentGeneratorGenerator
from griddly.wrappers import RenderWrapper


@pytest.fixture
def test_name(request):
    return request.node.name


def build_generator(test_name, yaml_file):
    wrapper_factory = GymWrapperFactory()
    egg = EnvironmentGeneratorGenerator(yaml_file=yaml_file)
    generator_yaml = egg.generate_env_yaml((10, 10))

    wrapper_factory.build_gym_from_yaml_string(
        test_name,
        yaml_string=generator_yaml,
        global_observer_type=gd.ObserverType.ASCII,
        player_observer_type=gd.ObserverType.ASCII,
    )

    env = gym.make(f"GDY-{test_name}-v0")
    env.reset()
    return env


def test_spider_nest_generator(test_name):
    yaml_file = "Single-Player/GVGAI/spider-nest.yaml"

    for i in range(10):
        genv = build_generator(test_name + f"{i}", yaml_file)

        player_observer = RenderWrapper(genv, render_mode="rgb_array")
        global_observer = RenderWrapper(
            genv, render_mode="rgb_array", observer="global"
        )

        # Place 10 Random Objects
        for i in range(0, 100):
            action = genv.action_space.sample()
            obs, reward, done, truncated, info = genv.step(action)

            player_ascii_string = player_observer.render()
            global_ascii_string = global_observer.render()

            assert player_ascii_string == global_ascii_string
