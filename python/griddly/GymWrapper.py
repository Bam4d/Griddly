import gym
import numpy as np
from gym.envs.registration import register

from griddly import GriddlyLoader, gd
from griddly.util.action_space import MultiAgentActionSpace
from griddly.util.observation_space import MultiAgentObservationSpace
from griddly.util.vector_visualization import Vector2RGB


class GymWrapper(gym.Env):
    metadata = {'render.modes': ['human', 'rgb_array']}

    def __init__(self, yaml_file=None, level=0, global_observer_type=gd.ObserverType.VECTOR,
                 player_observer_type=gd.ObserverType.VECTOR, max_steps=None, image_path=None, shader_path=None,
                 gdy=None, game=None):
        """
        Currently only supporting a single player (player 1 as defined in the environment yaml
        :param yaml_file:
        :param level:
        :param global_observer_type: the render mode for the global renderer
        :param player_observer_type: the render mode for the players
        """

        super(GymWrapper, self).__init__()

        # Set up multiple render windows so we can see what the AIs see and what the game environment looks like
        self._renderWindow = {}

        # If we are loading a yaml file
        if yaml_file is not None:
            self._is_clone = False
            loader = GriddlyLoader(image_path, shader_path)
            self.gdy = loader.load(yaml_file)
            self.game = self.gdy.create_game(global_observer_type)

            if max_steps is not None:
                self.gdy.set_max_steps(max_steps)

            if level is not None:
                self.game.load_level(level)

        # if we are loading a copy of the game
        elif gdy is not None and game is not None:
            self._is_clone = True
            self.gdy = gdy
            self.game = game

        self._players = []
        self.player_count = self.gdy.get_player_count()

        self._global_observer_type = global_observer_type
        self._player_observer_type = []

        for p in range(self.player_count):
            self._players.append(self.game.register_player(f'Player {p + 1}', player_observer_type))
            self._player_observer_type.append(player_observer_type)

        self._player_last_observation = []
        self._global_last_observation = None

        self.num_action_ids = {}

        self._enable_history = False

        self.game.init(self._is_clone)

    def get_state(self):
        return self.game.get_state()

    def get_tile_size(self, player=0):
        if player == 0:
            return self.game.get_tile_size()
        else:
            return self._players[player - 1].get_tile_size()

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
                raise ValueError(f'The supplied action is in the wrong format for this environment.\n\n'
                                 f'A valid example: {self.action_space.sample()}')

            reward, done, info = self._players[player_id].step_multi(action_data, True)

        elif len(action) == self.player_count:

            if np.ndim(action) == 1:
                if isinstance(action[0], list) or isinstance(action[0], np.ndarray):
                    # Multiple agents that can perform multiple actions in parallel
                    # Used in RTS games
                    reward = []
                    for p in range(self.player_count):
                        player_action = np.array(action[p], dtype=np.int)
                        final = p == self.player_count - 1
                        rew, done, info = self._players[p].step_multi(player_action, final)
                        reward.append(rew)
                else:
                    action = np.array(action, dtype=np.int32)
                    action_data = action.reshape(-1, 1)
                    reward, done, info = self.game.step_parallel(action_data)

            # Multiple agents executing actions in parallel
            # Used in multi-agent environments
            elif np.ndim(action) == 2:
                action_data = np.array(action, dtype=np.int32)
                reward, done, info = self.game.step_parallel(action_data)

        else:
            raise ValueError(f'The supplied action is in the wrong format for this environment.\n\n'
                             f'A valid example: {self.action_space.sample()}')

        for p in range(self.player_count):
            # Copy only if the environment is done (it will reset itself)
            # This is because the underlying data will be released
            self._player_last_observation[p] = np.array(self._players[p].observe(), copy=False)

        obs = self._player_last_observation[0] if self.player_count == 1 else self._player_last_observation

        if self._enable_history:
            info['History'] = self.game.get_history()
        return obs, reward, done, info

    def reset(self, level_id=None, level_string=None, global_observations=False):

        if level_string is not None:
            self.game.load_level_string(level_string)
        elif level_id is not None:
            self.game.load_level(level_id)

        self.game.reset()

        self.initialize_spaces()

        if global_observations:
            return {
                'global': self._global_last_observation,
                'player': self._player_last_observation[0] if self.player_count == 1 else self._player_last_observation
            }
        else:
            return self._player_last_observation[0] if self.player_count == 1 else self._player_last_observation

    def initialize_spaces(self):
        self._player_last_observation = []
        for p in range(self.player_count):
            self._player_last_observation.append(np.array(self._players[p].observe(), copy=False))

        self._global_last_observation = np.array(self.game.observe(), copy=False)

        self.player_observation_shape = self._player_last_observation[0].shape
        self.global_observation_shape = self._global_last_observation.shape

        self.global_observation_space = gym.spaces.Box(low=0, high=255, shape=self.global_observation_shape,
                                                       dtype=np.uint8)

        self._observation_shape = self._player_last_observation[0].shape
        observation_space = gym.spaces.Box(low=0, high=255, shape=self._observation_shape, dtype=np.uint8)

        if self.player_count > 1:
            observation_space = MultiAgentObservationSpace([observation_space for _ in range(self.player_count)])

        self.observation_space = observation_space

        self._vector2rgb = Vector2RGB(10, self._observation_shape[0])

        self.action_space = self._create_action_space()

    def render(self, mode='human', observer=0):

        if observer == 'global':
            observation = np.array(self.game.observe(), copy=False)
            if self._global_observer_type == gd.ObserverType.VECTOR:
                observation = self._vector2rgb.convert(observation)
        else:
            observation = self._player_last_observation[observer]
            if self._player_observer_type[observer] == gd.ObserverType.VECTOR:
                observation = self._vector2rgb.convert(observation)

        if mode == 'human':
            if self._renderWindow.get(observer) is None:
                from griddly.RenderTools import RenderWindow
                self._renderWindow[observer] = RenderWindow(observation.shape[1], observation.shape[2])
            self._renderWindow[observer].render(observation)

        return observation.swapaxes(0, 2)

    def get_keys_to_action(self):
        keymap = {
            (ord('a'),): 1,
            (ord('w'),): 2,
            (ord('d'),): 3,
            (ord('s'),): 4,
            (ord('e'),): 5
        }

        return keymap

    def close(self):
        for i, render_window in self._renderWindow.items():
            render_window.close()

        self._renderWindow = {}

    def __del__(self):
        self.close()

    def _create_action_space(self):

        self.player_count = self.gdy.get_player_count()
        self.action_input_mappings = self.gdy.get_action_input_mappings()

        self.grid_width = self.game.get_width()
        self.grid_height = self.game.get_height()

        self.avatar_object = self.gdy.get_avatar_object()

        self.has_avatar = self.avatar_object is not None and len(self.avatar_object) > 0

        self.action_names = self.gdy.get_action_names()
        self.action_count = len(self.action_names)
        self.default_action_name = self.action_names[0]

        action_space_parts = []

        if not self.has_avatar:
            action_space_parts.extend([self.grid_width, self.grid_height])

        if self.action_count > 1:
            action_space_parts.append(self.action_count)

        self.max_action_ids = 0
        for action_name, mapping in sorted(self.action_input_mappings.items()):
            if not mapping['Internal']:
                num_action_ids = len(mapping['InputMappings']) + 1
                self.num_action_ids[action_name] = num_action_ids
                if self.max_action_ids < num_action_ids:
                    self.max_action_ids = num_action_ids

        action_space_parts.append(self.max_action_ids)

        if len(action_space_parts) == 1:
            action_space = gym.spaces.Discrete(self.max_action_ids)
        else:
            action_space = gym.spaces.MultiDiscrete(action_space_parts)

        if self.player_count > 1:
            action_space = MultiAgentActionSpace([action_space for _ in range(self.player_count)])

        return action_space

    def clone(self):
        """
        Return an environment that is an executable copy of the current environment
        :return:
        """
        game_copy = self.game.clone()
        cloned_wrapper = GymWrapper(
            global_observer_type=self._global_observer_type,
            player_observer_type=self._player_observer_type[0],
            gdy=self.gdy,
            game=game_copy
        )

        cloned_wrapper.initialize_spaces()

        return cloned_wrapper


class GymWrapperFactory():

    def build_gym_from_yaml(self, environment_name, yaml_file, global_observer_type=gd.ObserverType.SPRITE_2D,
                            player_observer_type=gd.ObserverType.SPRITE_2D, level=None, max_steps=None):
        register(
            id=f'GDY-{environment_name}-v0',
            entry_point='griddly:GymWrapper',
            kwargs={
                'yaml_file': yaml_file,
                'level': level,
                'max_steps': max_steps,
                'global_observer_type': global_observer_type,
                'player_observer_type': player_observer_type
            }
        )
