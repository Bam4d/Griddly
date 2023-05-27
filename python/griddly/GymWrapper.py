import gymnasium as gym
import numpy as np
from typing import Optional, Union
from gymnasium.envs.registration import register
from gymnasium.spaces import Discrete, MultiDiscrete

from griddly import GriddlyLoader, gd
from griddly.util.action_space import MultiAgentActionSpace
from griddly.util.observation_space import (
    MultiAgentObservationSpace,
    EntityObservationSpace,
)
from griddly.util.vector_visualization import Vector2RGB


class _GymWrapperCache:
    """
    This class is used to cache properties of the Griddly environment so we dont have to recreate them on copies
    """

    def __init__(self):
        self.player_count = None
        self.level_count = None
        self.avatar_object = None
        self.action_input_mappings = None
        self.action_names = None
        self.player_observation_space = None
        self.global_observation_space = None
        self.action_space_parts = None
        self.max_action_ids = None
        self.num_action_ids = None
        self.action_space = None
        self.object_names = None
        self.variable_names = None
        self.vector2rgb = None

    def reset(self):
        self.__init__()


class GymWrapper(gym.Env):
    metadata = {"render_modes": ["human", "rgb_array"], "render_fps": 30}

    def __init__(
        self,
        yaml_file=None,
        yaml_string=None,
        level=0,
        global_observer_type=gd.ObserverType.VECTOR,
        player_observer_type=gd.ObserverType.VECTOR,
        max_steps=None,
        gdy=None,
        game=None,
        reset=True,
        render_mode="human",
        **kwargs,
    ):
        """
        Currently only supporting a single player (player 1 as defined in the environment yaml
        :param yaml_file:
        :param level:
        :param global_observer_type: the observer type for the global renderer
        :param player_observer_type: the observer type for the players
        """

        super(GymWrapper, self).__init__()

        self._enable_history = False
        self._cache = _GymWrapperCache()

        # Set up multiple render windows so we can see what the AIs see and what the game environment looks like
        self.render_mode = render_mode
        self._render_window = {}

        # If we are loading a yaml file
        if yaml_file is not None or yaml_string is not None:
            self._is_clone = False
            loader = GriddlyLoader()
            if yaml_file is not None:
                self.gdy = loader.load(yaml_file)
            else:
                self.gdy = loader.load_string(yaml_string)

            self._global_observer_type = self._get_observer_type(global_observer_type)
            self._global_observer_name = self._get_observer_name(global_observer_type)

            self.game = self.gdy.create_game(self._global_observer_name)

            if max_steps is not None:
                self.gdy.set_max_steps(max_steps)

            if level is not None:
                self.game.load_level(level)
                self.level_id = level

            self._player_last_observation = None
            self._global_last_observation = None

        # if we are loading a copy of the game
        elif gdy is not None and game is not None:
            if level is not None:
                self.level_id = level

            player_key = "player_last_observation"
            global_key = "global_last_observation"
            self._player_last_observation = (
                np.copy(kwargs[player_key]) if player_key in kwargs else None
            )
            self._global_last_observation = (
                np.copy(kwargs[global_key]) if global_key in kwargs else None
            )

            self._is_clone = True
            self.gdy = gdy
            self.game = game

            self._global_observer_type = self._get_observer_type(global_observer_type)
            self._global_observer_name = self._get_observer_name(global_observer_type)

        if isinstance(player_observer_type, list):
            self._player_observer_type = [
                self._get_observer_type(type_or_string)
                for type_or_string in player_observer_type
            ]
            self._player_observer_name = [
                self._get_observer_name(type_or_string)
                for type_or_string in player_observer_type
            ]
        else:
            self._player_observer_type = [
                self._get_observer_type(player_observer_type)
                for _ in range(self.player_count)
            ]
            self._player_observer_name = [
                self._get_observer_name(player_observer_type)
                for _ in range(self.player_count)
            ]

        self._players = []

        for p in range(self.player_count):
            self._players.append(
                self.game.register_player(
                    f"Player {p + 1}", self._player_observer_name[p]
                )
            )

        self.game.init(self._is_clone)
        if not self._is_clone and reset:
            self.reset()

    @property
    def player_count(self):
        if self._cache.player_count is None:
            self._cache.player_count = self.gdy.get_player_count()
        return self._cache.player_count

    @property
    def level_count(self):
        if self._cache.level_count is None:
            self._cache.level_count = self.gdy.get_level_count()
        return self._cache.level_count

    @property
    def avatar_object(self):
        if self._cache.avatar_object is None:
            self._cache.avatar_object = self.gdy.get_avatar_object()
        return self._cache.avatar_object

    @property
    def has_avatar(self):
        return bool(self.avatar_object)

    @property
    def action_input_mappings(self):
        if self._cache.action_input_mappings is None:
            self._cache.action_input_mappings = self.gdy.get_action_input_mappings()
        return self._cache.action_input_mappings

    @property
    def action_names(self):
        if self._cache.action_names is None:
            self._cache.action_names = self.gdy.get_action_names()
        return self._cache.action_names

    @property
    def action_count(self):
        return len(self.action_names)

    @property
    def default_action_name(self):
        return self.action_names[0]

    @property
    def object_names(self):
        if self._cache.object_names is None:
            self._cache.object_names = self.game.get_object_names()
        return self._cache.object_names

    @property
    def variable_names(self):
        if self._cache.variable_names is None:
            self._cache.variable_names = self.game.get_object_variable_names()
        return self._cache.variable_names

    @property
    def _vector2rgb(self):
        if self._cache.vector2rgb is None:
            self._cache.vector2rgb = Vector2RGB(10, len(self.object_names))
        return self._cache.vector2rgb

    @property
    def global_observation_space(self):
        if self._cache.global_observation_space is None:
            self._cache.global_observation_space = self._get_obs_space(
                self.game.get_global_observation_description(),
                self._global_observer_type,
            )
        return self._cache.global_observation_space

    @property
    def player_observation_space(self):
        if self._cache.player_observation_space is None:
            if self.player_count == 1:
                self._cache.player_observation_space = self._get_obs_space(
                    self._players[0].get_observation_description(),
                    self._player_observer_type[0],
                )
            else:
                observation_spaces = []
                for p in range(self.player_count):
                    observation_spaces.append(
                        self._get_obs_space(
                            self._players[p].get_observation_description(),
                            self._player_observer_type[p],
                        )
                    )
                self._cache.player_observation_space = MultiAgentObservationSpace(
                    observation_spaces
                )

        return self._cache.player_observation_space

    @property
    def observation_space(self):
        return self.player_observation_space

    @property
    def max_action_ids(self):
        if self._cache.max_action_ids is None:
            self._init_action_variable_cache()
        return self._cache.max_action_ids

    @property
    def num_action_ids(self):
        if self._cache.num_action_ids is None:
            self._init_action_variable_cache()
        return self._cache.num_action_ids

    @property
    def action_space_parts(self):
        if self._cache.action_space_parts is None:
            self._init_action_variable_cache()
        return self._cache.action_space_parts

    @property
    def action_space(self):
        if self._cache.action_space is None:
            self._cache.action_space = self._create_action_space()
        return self._cache.action_space

    @property
    def grid_width(self):
        return self.game.get_width()

    @property
    def grid_height(self):
        return self.game.get_height()

    def _get_observer_type(self, observer_type_or_string):
        if isinstance(observer_type_or_string, gd.ObserverType):
            return observer_type_or_string
        else:
            return self.gdy.get_observer_type(observer_type_or_string)

    def _get_observer_name(self, observer_type_or_string):
        if isinstance(observer_type_or_string, gd.ObserverType):
            if observer_type_or_string.name == "ASCII":
                return observer_type_or_string.name
            return observer_type_or_string.name.title().replace("_", "")
        else:
            return observer_type_or_string

    def _get_observation(self, observation, type):
        if type != gd.ObserverType.ENTITY:
            return np.array(observation, copy=True)
        else:
            return observation

    def get_state(self):
        return self.game.get_state()

    def get_tile_size(self, player=0):
        if player == 0:
            return self.game.get_global_observation_description()["TileSize"]
        else:
            return self._players[player - 1].get_global_observation_description()[
                "TileSize"
            ]

    def enable_history(self, enable=True):
        self._enable_history = enable
        self.game.enable_history(enable)

    def step(self, action):
        """
        Step for a particular player in the environment
        """

        player_id = 0

        # Simple agents executing single actions or multiple actions in a single time step
        if self.player_count == 1:
            action = np.array(action, dtype=np.int32)
            if np.ndim(action) == 0:
                action_data = action.reshape(1, -1)
            elif np.ndim(action) == 1:
                action_data = action.reshape(1, -1)
            elif np.ndim(action) == 2:
                action_data = np.array(action)
            else:
                raise ValueError(
                    f"The supplied action is in the wrong format for this environment.\n\n"
                    f"A valid example: {self.action_space.sample()}"
                )

            reward, done, truncated, info = self._players[player_id].step_multi(
                action_data, True
            )

        elif len(action) == self.player_count:
            processed_actions = []
            multi_action = False
            for a in action:
                if a is None:
                    processed_action = np.zeros(
                        (len(self.action_space_parts)), dtype=np.int32
                    )
                else:
                    processed_action = np.atleast_1d(np.array(a, dtype=np.int32))
                if len(processed_action.shape) > 1 and processed_action.shape[0] > 1:
                    multi_action = True
                processed_actions.append(processed_action)

            if not self.has_avatar and multi_action:
                # Multiple agents that can perform multiple actions in parallel
                # Used in RTS games
                reward = []
                for p in range(self.player_count):
                    player_action = processed_actions[p].reshape(
                        -1, len(self.action_space_parts)
                    )
                    final = p == self.player_count - 1
                    rew, done, truncated, info = self._players[p].step_multi(
                        player_action, final
                    )
                    reward.append(rew)

            # Multiple agents executing actions in parallel
            # Used in multi-agent environments
            else:
                action_data = np.array(processed_actions, dtype=np.int32)
                action_data = action_data.reshape(self.player_count, -1)
                reward, done, truncated, info = self.game.step_parallel(action_data)

        else:
            raise ValueError(
                f"The supplied action is in the wrong format for this environment.\n\n"
                f"A valid example: {self.action_space.sample()}"
            )
        # In the case where the environment is cloned, but no step has happened to replace the last obs,
        # we can do that here
        if self._player_last_observation is None:
            player_last_observation_list = []
            for p in range(self.player_count):
                player_last_observation_list.append(
                    self._get_observation(
                        self._players[p].observe(), self._player_observer_type[p]
                    )
                )
                self._player_last_observation = np.array(player_last_observation_list)
        else:
            for p in range(self.player_count):
                self._player_last_observation[p] = self._get_observation(
                    self._players[p].observe(), self._player_observer_type[p]
                )
        obs = (
            self._player_last_observation[0]
            if self.player_count == 1
            else self._player_last_observation
        )

        if self._enable_history:
            info["History"] = self.game.get_history()
        return obs, reward, done, truncated, info

    def reset(self, seed=None, options=None):
        if seed is None:
            seed = 100

        if options is None:
            options = {}

        super().reset(seed=seed, options=options)

        level_id: Optional[int] = options.get("level_id", None)
        level_string: Optional[str] = options.get("level_string", None)
        global_observations: bool = options.get("global_observations", False)

        if level_string is not None:
            self.game.load_level_string(level_string)
            self.level_id = "custom"
        elif level_id is not None:
            self.game.load_level(level_id)
            self.level_id = level_id

        self.game.seed(seed)
        self.game.reset()
        self._cache.reset()

        self._cache.action_space = self._create_action_space(seed=seed)

        player_last_observation_list = []
        for p in range(self.player_count):
            player_last_observation_list.append(
                self._get_observation(
                    self._players[p].observe(), self._player_observer_type[p]
                )
            )
        self._player_last_observation = player_last_observation_list

        if global_observations:
            self._global_last_observation = self._get_observation(
                self.game.observe(), self._global_observer_type
            )

            return {
                "global": self._global_last_observation,
                "player": self._player_last_observation[0]
                if self.player_count == 1
                else self._player_last_observation,
            }, {}
        else:
            return (
                self._player_last_observation[0]
                if self.player_count == 1
                else self._player_last_observation
            ), {}

    def _get_obs_space(self, description, type):
        if type != gd.ObserverType.ENTITY:
            return gym.spaces.Box(
                low=0, high=255, shape=description["Shape"], dtype=np.uint8
            )
        else:
            return EntityObservationSpace(description["Features"])

    def render(self):
        return self.render_observer(0, self.render_mode)

    def render_observer(self, observer=0, render_mode="human"):
        """

        Renders the output of the observer.

        Args:
            observer (int, optional): The observer to render. Defaults to 0.
            render_mode (str, optional): The format in which to render. 
                "human" will create a window and display the rendered observer, 
                "rgb_array" will return an array of rgb values. Defaults to "human".

        Returns:
            _type_: _description_
        """
        if observer == "global":
            observation = self._get_observation(
                self.game.observe(), self._global_observer_type
            )
            if self._global_observer_type == gd.ObserverType.VECTOR:
                observation = self._vector2rgb.convert(observation)
            if self._global_observer_type == gd.ObserverType.ASCII:
                observation = (
                    observation.swapaxes(2, 0)
                    .reshape(-1, observation.shape[0] * observation.shape[1])
                    .view("c")
                )
                ascii_string = "".join(
                    np.column_stack(
                        (observation, np.repeat(["\n"], observation.shape[0]))
                    )
                    .flatten()
                    .tolist()
                )
                return ascii_string

        else:
            observation = self._player_last_observation[observer]
            if self._player_observer_type[observer] == gd.ObserverType.VECTOR:
                observation = self._vector2rgb.convert(observation)
            if self._player_observer_type[observer] == gd.ObserverType.ASCII:
                observation = (
                    observation.swapaxes(2, 0)
                    .reshape(-1, observation.shape[0] * observation.shape[1])
                    .view("c")
                )
                ascii_string = "".join(
                    np.column_stack(
                        (observation, np.repeat(["\n"], observation.shape[0]))
                    )
                    .flatten()
                    .tolist()
                )
                return ascii_string

        if render_mode == "human":
            if self._render_window.get(observer) is None:
                from griddly.util.render_tools import RenderToWindow

                self._render_window[observer] = RenderToWindow(
                    observation.shape[1], observation.shape[2]
                )
            self._render_window[observer].render(observation)

        return observation.swapaxes(0, 2)

    def get_keys_to_action(self):
        return {
            (ord("a"),): 1,
            (ord("w"),): 2,
            (ord("d"),): 3,
            (ord("s"),): 4,
            (ord("e"),): 5,
        }

    def close(self):
        if hasattr(self, "_render_window"):
            for i, render_window in self._render_window.items():
                render_window.close()

        self._render_window = {}

    def __del__(self):
        self.close()
        if hasattr(self, "game"):
            self.game.release()

    def _create_action_space(self, seed=None):
        if len(self.action_space_parts) == 1:
            action_space = Discrete(self.max_action_ids, seed=seed)
        else:
            action_space = MultiDiscrete(self.action_space_parts, seed=seed)

        if self.player_count > 1:
            action_space = MultiAgentActionSpace(
                [action_space for _ in range(self.player_count)], seed=seed
            )

        return action_space

    def _init_action_variable_cache(self):
        self._cache.action_space_parts = []

        if not self.has_avatar:
            self._cache.action_space_parts.extend([self.grid_width, self.grid_height])

        if self.action_count > 1:
            self._cache.action_space_parts.append(self.action_count)

        self._cache.num_action_ids = {}
        max_action_ids = 0

        for action_name, mapping in sorted(self.action_input_mappings.items()):
            if not mapping["Internal"]:
                num_action_ids = len(mapping["InputMappings"]) + 1
                self._cache.num_action_ids[action_name] = num_action_ids
                if max_action_ids < num_action_ids:
                    max_action_ids = num_action_ids

        self._cache.max_action_ids = max_action_ids
        self._cache.action_space_parts.append(max_action_ids)

    def clone(self):
        """
        Return an environment that is an executable copy of the current environment
        :return:
        """
        return GymWrapper(
            level=self.level_id,
            gdy=self.gdy,
            game=self.game.clone(),
            global_observer_type=self._global_observer_type,
            player_observer_type=self._player_observer_type,
            player_last_observation=self._player_last_observation,
            global_last_observation=self._global_last_observation,
        )


class GymWrapperFactory:
    def build_gym_from_yaml(
        self,
        environment_name,
        yaml_file,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        player_observer_type=gd.ObserverType.SPRITE_2D,
        level=None,
        max_steps=None,
    ):
        register(
            id=f"GDY-{environment_name}-v0",
            entry_point="griddly:GymWrapper",
            kwargs={
                "yaml_file": yaml_file,
                "level": level,
                "max_steps": max_steps,
                "global_observer_type": global_observer_type,
                "player_observer_type": player_observer_type,
            },
        )

    def build_gym_from_yaml_string(
        self,
        environment_name,
        yaml_string,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        player_observer_type=gd.ObserverType.SPRITE_2D,
        level=None,
        max_steps=None,
    ):
        register(
            id=f"GDY-{environment_name}-v0",
            entry_point="griddly:GymWrapper",
            kwargs={
                "yaml_string": yaml_string,
                "level": level,
                "max_steps": max_steps,
                "global_observer_type": global_observer_type,
                "player_observer_type": player_observer_type,
            },
        )
