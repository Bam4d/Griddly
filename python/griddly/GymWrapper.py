import colorsys

import gym
import numpy as np
from griddly.util.vector_visualization import Vector2RGB
from gym import Space
from gym.envs.registration import register
from gym.spaces import MultiDiscrete, Discrete

from griddly import GriddlyLoader, gd


class GriddlyActionSpace(Space):

    def __init__(self, player_count, action_names, action_input_mappings, grid_width, grid_height, has_avatar):

        self.available_action_input_mappings = {}
        self.action_names = action_names
        self.action_space_dict = {}
        for k, mapping in action_input_mappings.items():
            if not mapping['Internal']:
                num_actions = len(mapping['InputMappings']) + 1
                self.available_action_input_mappings[k] = mapping
                if has_avatar:
                    self.action_space_dict[k] = gym.spaces.Discrete(num_actions)
                else:
                    self.action_space_dict[k] = gym.spaces.MultiDiscrete([grid_width, grid_height, num_actions])

        self.available_actions_count = len(self.action_names)

        self.player_count = player_count
        self.player_space = gym.spaces.Discrete(player_count)

    def sample(self):

        sampled_action_def = np.random.choice(self.action_names)
        sampled_action_space = self.action_space_dict[sampled_action_def].sample()
        sampled_player = self.player_space.sample()

        return {
            'player': sampled_player,
            sampled_action_def: sampled_action_space
        }


class GymWrapper(gym.Env):
    metadata = {'render.modes': ['human', 'rgb_array']}

    def __init__(self, yaml_file=None, level=0, global_observer_type=gd.ObserverType.SPRITE_2D,
                 player_observer_type=gd.ObserverType.SPRITE_2D, max_steps=None, image_path=None, shader_path=None,
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
            self._players.append(self.game.register_player(f'Player {p+1}', player_observer_type))
            self._player_observer_type.append(player_observer_type)

        self._player_last_observation = {}

        self.game.init(self._is_clone)

    def get_state(self):
        return self.game.get_state()

    def get_tile_size(self, player=0):
        if player == 0:
            return self.game.get_tile_size()
        else:
            return self._players[player - 1].get_tile_size()

    def enable_history(self, enable=True):
        self.game.enable_history(enable)

    def step(self, action):
        """
        Step for a particular player in the environment

        :param action:
        :return:
        """

        # TODO: support more than 1 action at at time
        # TODO: support batches for parallel environment processing

        player_id = 0

        if isinstance(self.action_space, Discrete) or isinstance(self.action_space, MultiDiscrete):
            action_name = self.default_action_name

            if isinstance(action, int) or np.ndim(action) == 0:
                action_data = [action]
            elif isinstance(action, list) or isinstance(action, np.ndarray):
                action_data = action
            else:
                raise ValueError(f'The supplied action is in the wrong format for this environment.\n\n'
                                 f'A valid example: {self.action_space.sample()}')

        elif isinstance(self.action_space, GriddlyActionSpace):

            if isinstance(action, dict):

                player_id = action['player']
                del action['player']

                assert len(action) == 1, "Only 1 action can be performed on each step."

                action_name = next(iter(action))
                action = action[action_name]
                if isinstance(action, int) or np.isscalar(action):
                    action_data = [action]
                elif isinstance(action, list) or isinstance(action, np.ndarray):
                    action_data = action
            else:
                raise ValueError(f'The supplied action is in the wrong format for this environment.\n\n'
                                 f'A valid example: {self.action_space.sample()}')

        reward, done, info = self._players[player_id].step(action_name, action_data, True)
        self._player_last_observation[player_id] = np.array(self._players[player_id].observe(), copy=False)
        return self._player_last_observation[player_id], reward, done, info

    def reset(self, level_id=None, level_string=None):

        if level_string is not None:
            self.game.load_level_string(level_string)
        elif level_id is not None:
            self.game.load_level(level_id)

        self.game.reset()

        return self.initialize_observation_spaces()

    def initialize_observation_spaces(self):
        for p in range(self.player_count):
            self._player_last_observation[p] = np.array(self._players[p].observe(), copy=False)

        global_observation = np.array(self.game.observe(), copy=False)

        self.player_observation_shape = self._player_last_observation[0].shape
        self.global_observation_shape = global_observation.shape

        self._observation_shape = self._player_last_observation[0].shape
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=self._observation_shape, dtype=np.uint8)

        self._vector2rgb = Vector2RGB(10, self._observation_shape[0])

        self.action_space = self._create_action_space()

        return global_observation

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

        grid_width = self.game.get_width()
        grid_height = self.game.get_height()

        self.avatar_object = self.gdy.get_avatar_object()

        has_avatar = self.avatar_object is not None and len(self.avatar_object) > 0

        num_mappings = 0
        self.action_names = self.gdy.get_action_names()

        for k, mapping in self.action_input_mappings.items():
            if not mapping['Internal']:
                num_mappings += 1

        # If there's only a single player and a single action mapping then just return a simple discrete space
        if num_mappings == 1:
            self.default_action_name = self.action_names[0]

            if self.player_count == 1:
                mapping = self.action_input_mappings[self.default_action_name]
                num_actions = len(mapping['InputMappings']) + 1

                if has_avatar:
                    return gym.spaces.Discrete(num_actions)
                else:
                    return gym.spaces.MultiDiscrete([grid_width, grid_height, num_actions])

        return GriddlyActionSpace(
            self.player_count,
            self.action_names,
            self.action_input_mappings,
            grid_width,
            grid_height,
            self.avatar_object
        )

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

        cloned_wrapper.initialize_observation_spaces()

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
