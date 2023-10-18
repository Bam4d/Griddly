import os

import yaml

from griddly.gym import GymWrapperFactory


def preload_default_envs() -> None:
    module_path = os.path.dirname(os.path.realpath(__file__))
    game_path = os.path.join(module_path, "resources", "games")

    wrapper_factory = GymWrapperFactory()

    for directory_path, directory_names, filenames in os.walk(game_path):
        for filename in filenames:
            if filename.endswith(".yaml"):
                full_path = os.path.join(directory_path, filename)
                with open(full_path, "r") as game_file:
                    gdy = yaml.load(game_file, Loader=yaml.FullLoader)
                    environment = gdy["Environment"]

                    formatted_env_name = environment["Name"].replace(" ", "-")
                    wrapper_factory.build_gym_from_yaml(
                        formatted_env_name, full_path, level=0
                    )


preload_default_envs()
