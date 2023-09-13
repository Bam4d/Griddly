import os
from typing import Any, Dict, List, Optional, Union

import gymnasium as gym
import numpy as np
import yaml

from griddly import gd
from griddly.gym import GymWrapper, GymWrapperFactory


class EnvironmentGeneratorGenerator:
    def __init__(self, yaml_file: str, gdy_path: Optional[str] = None) -> None:
        module_path = os.path.dirname(os.path.realpath(__file__))
        self._gdy_path = (
            os.path.realpath(os.path.join(module_path, "../", "resources", "games"))
            if gdy_path is None
            else gdy_path
        )
        self._input_yaml_file = self._get_full_path(yaml_file)

    def _get_full_path(self, gdy_path: str) -> str:
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
                os.path.join(self._gdy_path + "../../../../resources/games", gdy_path)
            )
        )
        return fullpath

    def generate_env_yaml(self, level_shape: List[int]) -> str:
        assert len(level_shape) == 2, "Level shape must be 2D"

        level_generator_gdy: Dict[str, Any] = {}
        with open(self._input_yaml_file, "r") as fs:
            self._gdy = yaml.load(fs, Loader=yaml.FullLoader)

        objects = [o for o in self._gdy["Objects"] if "MapCharacter" in o]
        environment = self._gdy["Environment"]

        # Create the placement actions
        actions = []
        for obj in objects:
            object_name = obj["Name"]
            place_action = {
                "InputMapping": {
                    "Inputs": {
                        "1": {"Description": f'Places objects of type "{object_name}"'}
                    }
                },
                "Name": f"place_{object_name.lower()}",
                "Behaviours": [
                    {"Src": {"Object": "_empty", "Commands": [{"spawn": object_name}]}}
                ],
            }
            actions.append(place_action)

        level_generator_gdy["Actions"] = actions

        # Copy the Objects
        level_generator_gdy["Objects"] = [
            {
                "Name": o["Name"],
                "MapCharacter": o["MapCharacter"],
                "Observers": o["Observers"],
            }
            for o in objects
        ]

        # Generate a default empty level
        empty_level = np.empty(level_shape, dtype="str")
        empty_level[:] = "."

        level_0_string = "\n".join(["   ".join(list(r)) for r in empty_level])

        # Create the environment template
        level_generator_gdy["Environment"] = {
            "Name": f'{environment["Name"]} Generator',
            "Description": f'Level Generator environment for {environment["Name"]}',
            "Observers": {
                k: v
                for k, v in environment["Observers"].items()
                if k in ["Sprite2D", "Isometric"]
            },
            "Player": {
                "Observer": {
                    "TrackAvatar": False,
                    "Height": level_shape[1],
                    "Width": level_shape[0],
                    "OffsetX": 0,
                    "OffsetY": 0,
                }
            },
            "Levels": [level_0_string],
        }

        return yaml.dump(level_generator_gdy)

    def generate_env(
        self,
        size: List[int],
        global_observer_type: Union[gd.ObserverType, str] = gd.ObserverType.VECTOR,
        player_observer_type: Union[
            List[Union[gd.ObserverType, str]], Union[gd.ObserverType, str]
        ] = gd.ObserverType.VECTOR,
        max_steps: Optional[int] = None,
        render_mode: str = "human",
    ) -> GymWrapper:
        env_yaml = self.generate_env_yaml(size)

        return GymWrapper(
            yaml_string=env_yaml,
            global_observer_type=global_observer_type,
            player_observer_type=player_observer_type,
            max_steps=max_steps,
            render_mode=render_mode,
        )


if __name__ == "__main__":
    wrapper_factory = GymWrapperFactory()
    yaml_file = "Single-Player/GVGAI/sokoban.yaml"

    egg = EnvironmentGeneratorGenerator(yaml_file=yaml_file)

    for i in range(100):
        generator_yaml = egg.generate_env_yaml([10, 10])

        env_name = f"test_{i}"
        wrapper_factory.build_gym_from_yaml_string(
            env_name,
            yaml_string=generator_yaml,
            # TODO: Change this to ASCII observer when its ready
            global_observer_type=gd.ObserverType.VECTOR,
            player_observer_type=gd.ObserverType.VECTOR,
        )

        env = gym.make(f"GDY-{env_name}-v0")
        env.reset()
        # env = ValidActionSpaceWrapper(env)

        # visualization = env.render(observer=0, mode='rgb_array')
        # video_recorder = VideoRecorder()
        # video_recorder.start('generator_video_test.mp4', visualization.shape)

        # Place 10 Random Objects
        for i in range(0, 100):
            action = env.action_space.sample()
            obs, reward, done, truncated, info = env.step(action)

            # state = env.get_state()

            # visual = env.render(observer=0, mode='rgb_array')
            # video_recorder.add_frame(visual)
