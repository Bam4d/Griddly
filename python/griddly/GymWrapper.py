import copy
import gym
import numpy as np
import time
from dataclasses import dataclass
from functools import lru_cache
from gym.envs.registration import register

from griddly import GriddlyLoader, gd
from griddly.util.action_space import MultiAgentActionSpace
from griddly.util.observation_space import MultiAgentObservationSpace, EntityObservationSpace
from griddly.util.vector_visualization import Vector2RGB

@dataclass
class _GymWrapperCache:
    player_count = None
    level_count = None
    avatar_object = None
    action_input_mappings = None
    action_names = None
    player_observation_space = None
    global_observation_space = None
    action_space_parts = None
    max_action_ids = 0
    num_action_ids = {}
    action_space = None
    object_names = None
    variable_names = None
    vector2rgb = None

    def reset(self):
        self.player_count = None
        self.level_count = None
        self.avatar_object = None
        self.action_input_mappings = None
        self.action_names = None
        self.player_observation_space = None
        self.global_observation_space = None
        self.action_space_parts = None
        self.max_action_ids = 0
        self.num_action_ids = {}
        self.action_space = None
        self.object_names = None
        self.variable_names = None
        self.vector2rgb = None


@dataclass
class _GymWrapperState:
    level_id = 0
    global_observer_type = None
    global_observer_name = None
    player_observer_type = None
    player_observer_name = None
    player_last_observation = None
    global_last_observation = None
    renderWindow = {}

    def print(self):
        print('level_id', self.level_id)
        print('global_observer_type', self.global_observer_type)
        print('global_observer_name', self.global_observer_name)
        print('player_observer_type', self.player_observer_type)
        print('player_observer_name', self.player_observer_name)
        print('player_last_observation', self.player_last_observation)
        print('global_last_observation', self.global_last_observation)
        print('renderWindow', self.renderWindow)

class GymWrapper(gym.Env):
    metadata = {"render.modes": ["human", "rgb_array"]}

    def __init__(
            self,
            yaml_file=None,
            yaml_string=None,
            level=0,
            global_observer_type=gd.ObserverType.VECTOR,
            player_observer_type=gd.ObserverType.VECTOR,
            max_steps=None,
            gdy_path=None,
            image_path=None,
            shader_path=None,
            gdy=None,
            game=None,
            existing_wrapper_state=None,
            **kwargs,
    ):
        """
        Currently only supporting a single player (player 1 as defined in the environment yaml
        :param yaml_file:
        :param level:
        :param global_observer_type: the render mode for the global renderer
        :param player_observer_type: the render mode for the players
        """

        super(GymWrapper, self).__init__()

        self._enable_history = False

        self._cache = _GymWrapperCache()

        # If we are loading a yaml file
        if yaml_file is not None or yaml_string is not None:
            self._state = _GymWrapperState()
            # Set up multiple render windows so we can see what the AIs see and what the game environment looks like
            self._state.renderWindow = {}

            self._is_clone = False
            loader = GriddlyLoader(gdy_path, image_path, shader_path)
            if yaml_file is not None:
                self.gdy = loader.load(yaml_file)
            else:
                self.gdy = loader.load_string(yaml_string)

            self._state.global_observer_type = self._get_observer_type(global_observer_type)
            self._state.global_observer_name = self._get_observer_name(global_observer_type)

            self.game = self.gdy.create_game(self._state.global_observer_name)

            if max_steps is not None:
                self.gdy.set_max_steps(max_steps)

            if level is not None:
                self.game.load_level(level)
                self._state.level_id = level

            if isinstance(player_observer_type, list):
                self._state.player_observer_type = [self._get_observer_type(type_or_string) for type_or_string in
                                            player_observer_type]
                self._state.player_observer_name = [self._get_observer_name(type_or_string) for type_or_string in
                                            player_observer_type]
            else:
                self._state.player_observer_type = [self._get_observer_type(player_observer_type) for _ in
                                            range(self.player_count)]
                self._state.player_observer_name = [self._get_observer_name(player_observer_type) for _ in
                                            range(self.player_count)]

            self._state.player_last_observation = []
            self._state.global_last_observation = None

            self._state.num_action_ids = {}

        # if we are loading a copy of the game
        elif gdy is not None and game is not None:
            #existing_wrapper_state.print()
            self._state = copy.deepcopy(existing_wrapper_state)
            self._is_clone = True
            self.gdy = gdy
            self.game = game
        
        self._players = []
        
        for p in range(self.player_count):
            self._players.append(
                self.game.register_player(f"Player {p + 1}", self._state.player_observer_name[p])
            )

        self.game.init(self._is_clone)


    @property
    def player_count(self):
        if not self._cache.player_count:
            self._cache.player_count = self.gdy.get_player_count()
        return self._cache.player_count

    @property
    def level_count(self):
        if not self._cache.level_count:
            self._cache.level_count = self.gdy.get_level_count()
        return self._cache.level_count

    @property
    def avatar_object(self):
        if not self._cache.avatar_object:
            self._cache.avatar_object = self.gdy.get_avatar_object()
        return self._cache.avatar_object

    @property
    def has_avatar(self):
        return bool(self.avatar_object)

    @property
    def action_input_mappings(self):
        if not self._cache.action_input_mappings:
            self._cache.action_input_mappings = self.gdy.get_action_input_mappings()
        return self._cache.action_input_mappings

    @property
    def action_names(self):
        if not self._cache.action_names:
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
        if not self._cache.object_names:
            self._cache.object_names = self.game.get_object_names()
        return self._cache.object_names

    @property
    def variable_names(self):
        if not self._cache.variable_names:
            self._cache.variable_names = self.game.get_object_variable_names()
        return self._cache.variable_names

    @property
    def vector2rgb(self):
        if not self._cache.vector2rgb:
            self._cache.vector2rgb = Vector2RGB(10, len(self.object_names))
        return self._cache.vector2rgb

    @property
    def global_observation_space(self):
        if not self._cache.global_observation_space:
            self._cache.global_observation_space = self._get_obs_space(
                self.game.get_global_observation_description(),
                self._state.global_observer_type
            )
        return self._cache.global_observation_space

    @property
    def player_observation_space(self):
        if not self._cache.player_observation_space:
            if self.player_count == 1:
                self._cache.player_observation_space = self._get_obs_space(
                    self._players[0].get_observation_description(),
                    self._state.player_observer_type[0]
                )
            else:
                observation_spaces = []
                for p in range(self.player_count):
                    observation_description = self._players[p].get_observation_description()
                    observation_spaces.append(
                        self._get_obs_space(
                            self._players[p].get_observation_description(),
                            self._state.player_observer_type[p]
                        )
                    )
                self._cache.player_observation_space = MultiAgentObservationSpace(observation_spaces)

        return self._cache.player_observation_space

    @property
    def observation_space(self):
        return self.player_observation_space

    @property
    def max_action_ids(self):
        if not self._cache.max_action_ids:
            self._init_action_variable_cache()
        return self._cache.max_action_ids

    @property
    def num_action_ids(self):
        if not self._cache.num_action_ids:
            self._init_action_variable_cache()
        return self._cache.num_action_ids

    @property
    def action_space_parts(self):
        if not self._cache.action_space_parts:
            self._init_action_variable_cache()
        return self._cache.action_space_parts

    @property
    def action_space(self):
        if not self._cache.action_space:
            self._cache.action_space = self._create_action_space()
        return self._cache.action_space

    @property
    def level_id(self):
        return self._state.level_id

    @property
    def _player_last_observation(self):
        return self._state.player_last_observation

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
            if observer_type_or_string.name == 'ASCII':
                return observer_type_or_string.name
            return observer_type_or_string.name.title().replace('_','')
        else:
            return observer_type_or_string

    def _get_observation(self, observation, type):
        if type != gd.ObserverType.ENTITY:
            return np.array(observation, copy=False)
        else:
            return observation

    def get_state(self):
        return self.game.get_state()

    def get_tile_size(self, player=0):
        if player == 0:
            return self.game.get_global_observation_description()["TileSize"]
        else:
            return self._players[player - 1].get_global_observation_description()["TileSize"]

    def enable_history(self, enable=True):
        self._enable_history = enable
        self.game.enable_history(enable)

    def step(self, action):
        """
        Step for a particular player in the environment
        """

        player_id = 0
        reward = None
        done = False
        info = {}

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

            reward, done, info = self._players[player_id].step_multi(action_data, True)

        elif len(action) == self.player_count:

            processed_actions = []
            multi_action = False
            for a in action:
                if a is None:
                    processed_action = np.zeros(
                        (len(self.action_space_parts)), dtype=np.int32
                    )
                else:
                    processed_action = np.array(a, dtype=np.int32)
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
                    rew, done, info = self._players[p].step_multi(player_action, final)
                    reward.append(rew)

            # Multiple agents executing actions in parallel
            # Used in multi-agent environments
            else:
                action_data = np.array(processed_actions, dtype=np.int32)
                action_data = action_data.reshape(self.player_count, -1)
                reward, done, info = self.game.step_parallel(action_data)

        else:
            raise ValueError(
                f"The supplied action is in the wrong format for this environment.\n\n"
                f"A valid example: {self.action_space.sample()}"
            )
        # In the case where the environment is cloned, but no step has happened to replace the last obs,
        # we can do that here
        if len(self._state.player_last_observation) == 0:
            for p in range(self.player_count):
                self._state.player_last_observation.append(
                    self._get_observation(self._players[p].observe(), self._state.player_observer_type[p])
                )
        else:
            for p in range(self.player_count):
                self._state.player_last_observation[p] = self._get_observation(self._players[p].observe(),
                                                                         self._state.player_observer_type[p])

        obs = (
            self._state.player_last_observation[0]
            if self.player_count == 1
            else self._state.player_last_observation
        )

        if self._enable_history:
            info["History"] = self.game.get_history()
        return obs, reward, done, info

    def reset(self, level_id=None, level_string=None, global_observations=False):
        if level_string is not None:
            self.game.load_level_string(level_string)
            self._state.level_id = "custom"
        elif level_id is not None:
            self.game.load_level(level_id)
            self._state.level_id = level_id

        
        old_np_random = self._extract_np_random(self._cache.action_space) if self._cache.action_space else None

        self.game.reset()
        self._cache.reset()

        print(old_np_random)

        self._state.player_last_observation = []
        self._cache.action_space = self._create_action_space(existing_np_random=old_np_random)

        for p in range(self.player_count):
            self._state.player_last_observation.append(
                self._get_observation(self._players[p].observe(), self._state.player_observer_type[p]))

        if global_observations:
            self._state.global_last_observation = self._get_observation(self.game.observe(), self._state.global_observer_type)

            return {
                "global": self._state.global_last_observation,
                "player": self._state.player_last_observation[0]
                if self.player_count == 1
                else self._state.player_last_observation,
            }
        else:
            return (
                self._state.player_last_observation[0]
                if self.player_count == 1
                else self._state.player_last_observation
            )

    def _get_obs_space(self, description, type):
        if type != gd.ObserverType.ENTITY:
            return gym.spaces.Box(low=0, high=255, shape=description["Shape"], dtype=np.uint8)
        else:
            return EntityObservationSpace(description["Features"])

    def render(self, mode="human", observer=0):
        if observer == "global":
            observation = self._get_observation(self.game.observe(), self._state.global_observer_type)
            if self._state.global_observer_type == gd.ObserverType.VECTOR:
                observation = self.vector2rgb.convert(observation)
            if self._state.global_observer_type == gd.ObserverType.ASCII:
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
            # In the case where the environment is cloned, but no step has happened to replace the last obs,
            # we can do that here
            if len(self._state.player_last_observation) == 0:
                for p in range(self.player_count):
                    self._state.player_last_observation.append(
                        self._get_observation(self._players[p].observe(), self._state.player_observer_type[p])
                    )

            observation = self._state.player_last_observation[observer]
            if self._state.player_observer_type[observer] == gd.ObserverType.VECTOR:
                observation = self.vector2rgb.convert(observation)
            if self._state.player_observer_type[observer] == gd.ObserverType.ASCII:
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

        if mode == "human":
            if self._state.renderWindow.get(observer) is None:
                from griddly.RenderTools import RenderWindow

                self._state.renderWindow[observer] = RenderWindow(
                    observation.shape[1], observation.shape[2]
                )
            self._state.renderWindow[observer].render(observation)

        return observation.swapaxes(0, 2)

    def get_keys_to_action(self):
        keymap = {
            (ord("a"),): 1,
            (ord("w"),): 2,
            (ord("d"),): 3,
            (ord("s"),): 4,
            (ord("e"),): 5,
        }

        return keymap

    def close(self):
        for i, render_window in self._state.renderWindow.items():
            render_window.close()

        self._state.renderWindow = {}

        self.game.release()

    def __del__(self):
        self.close()

    def _create_action_space(self, existing_np_random=None):
        if len(self.action_space_parts) == 1:
            action_space = gym.spaces.Discrete(self.max_action_ids)
        else:
            action_space = gym.spaces.MultiDiscrete(self.action_space_parts)

        if self.player_count > 1:
            action_space = MultiAgentActionSpace(
                [action_space for _ in range(self.player_count)]
            )
            if existing_np_random is not None:
                for np_random, space in zip(existing_np_random, action_space):
                    space._np_random = np_random
        else:
            if existing_np_random is not None:
                action_space._np_random = existing_np_random

        return action_space
    
    def _extract_np_random(self, action_space):
        if self.player_count > 1:
            return [space._np_random for space in action_space]
        else:
            return action_space._np_random

    
    def _init_action_variable_cache(self):
        self._cache.action_space_parts = []

        if not self.has_avatar:
            self._cache.action_space_parts.extend([self.grid_width, self.grid_height])

        if self.action_count > 1:
            self._cache.action_space_parts.append(self.action_count)

        for action_name, mapping in sorted(self.action_input_mappings.items()):
            if not mapping["Internal"]:
                num_action_ids = len(mapping["InputMappings"]) + 1
                self._cache.num_action_ids[action_name] = num_action_ids
                if self._cache.max_action_ids < num_action_ids:
                    self._cache.max_action_ids = num_action_ids

        self._cache.action_space_parts.append(self.max_action_ids)


    def clone(self):
        """
        Return an environment that is an executable copy of the current environment
        :return:
        """
        return GymWrapper(
            gdy=self.gdy,
            game=self.game.clone(),
            existing_wrapper_state=self._state
        )

    def seed(self, seed=None):
        if seed is None:
            seed = 1234

        self.game.seed(seed)
        self.action_space.seed(seed)
        self.observation_space.seed(seed)


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
