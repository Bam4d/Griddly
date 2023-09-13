import os
from typing import Any, Dict

import yaml

from griddly import gd
from griddly.gym import GymWrapperFactory


class GriddlyLoader:
    def __init__(self) -> None:
        module_path = os.path.dirname(os.path.realpath(__file__))
        self._image_path = os.path.join(module_path, "resources", "images")
        self._shader_path = os.path.join(module_path, "resources", "shaders")
        self._gdy_path = os.path.join(module_path, "resources", "games")

        self._gdy_reader = gd.GDYLoader(
            self._gdy_path, self._image_path, self._shader_path
        )

    def get_full_path(self, gdy_path: str) -> str:
        # Assume the file is relative first and if not, try to find it in the pre-defined games
        fullpath = (
            gdy_path
            if os.path.exists(gdy_path)
            else os.path.join(self._gdy_path, gdy_path)
        )
        # (for debugging only) look in parent directory resources because we might not have built the latest version
        fullpath = (
            fullpath
            if os.path.exists(fullpath)
            else os.path.realpath(
                os.path.join(
                    self._gdy_path + "../../../../../resources/games", gdy_path
                )
            )
        )
        return fullpath

    def load(self, gdy_path: str) -> gd.GDY:
        return self._gdy_reader.load(self.get_full_path(gdy_path))

    def load_string(self, yaml_string: str) -> gd.GDY:
        return self._gdy_reader.load_string(yaml_string)

    def load_gdy(self, gdy_path: str) -> Dict[str, Any]:
        with open(self.get_full_path(gdy_path)) as gdy_file:
            return yaml.load(gdy_file, Loader=yaml.SafeLoader)  # type: ignore


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
