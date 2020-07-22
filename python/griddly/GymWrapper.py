import gym
import numpy as np
from gym import Space
from gym.envs.registration import register

from griddly import GriddlyLoader, gd

class GriddlyActionSpace(Space):

    def __init__(self, grid):

        self.player_count = grid.get_player_count()
        self.action_input_mappings = grid.get_action_input_mappings()

        self._grid_width = grid.get_width()
        self._grid_height = grid.get_height()

        self.avatar_object = grid.get_avatar_object()

        # Enable history by default
        grid.enable_history(True)

        self._has_avatar = self.avatar_object is not None and len(self.avatar_object) > 0

        self.available_action_input_mappings = {}
        self.action_names = []
        self.action_space_dict = {}
        for k, mapping in sorted(self.action_input_mappings.items()):
            if not mapping['Internal']:
                num_actions = len(mapping['InputMappings']) + 1
                self.available_action_input_mappings[k] = mapping
                self.action_names.append(k)
                if self._has_avatar:
                    self.action_space_dict[k] = gym.spaces.MultiDiscrete([num_actions])
                else:
                    self.action_space_dict[k] = gym.spaces.MultiDiscrete([self._grid_width, self._grid_height, num_actions])

        self.available_actions_count = len(self.action_names)

        self.player_space = gym.spaces.Discrete(self.player_count)

    def sample(self):

        sampled_action_def = np.random.choice(self.action_names)
        sampled_action_space = self.action_space_dict[sampled_action_def].sample()
        sampled_player = self.player_space.sample()

        return {
            'player': sampled_player,
            sampled_action_def: sampled_action_space
        }

class GymWrapper(gym.Env):

    def __init__(self, yaml_file, level=0, global_observer_type=gd.ObserverType.SPRITE_2D,
                 player_observer_type=gd.ObserverType.SPRITE_2D, tile_size=None, image_path=None, shader_path=None):
        """
        Currently only supporting a single player (player 1 as defined in the environment yaml
        :param yaml_file:
        :param level:
        :param global_observer_type: the render mode for the global renderer
        :param player_observer_type: the render mode for the players
        """

        # Set up multiple render windows so we can see what the AIs see and what the game environment looks like
        self._renderWindow = {}

        loader = GriddlyLoader(image_path, shader_path)

        self._grid = loader.load_game(yaml_file)
        self._grid.load_level(level)

        self._players = []
        self.player_count = self._grid.get_player_count()

        if tile_size is not None:
            self._grid.set_tile_size(tile_size)

        self.game = self._grid.create_game(global_observer_type)

        for p in range(1, self.player_count + 1):
            self._players.append(self.game.register_player(f'Player {p}', player_observer_type))

        self._last_observation = {}

        self.game.init()

    def step(self, action):
        """
        Step for a particular player in the environment

        :param action:
        :return:
        """

        # TODO: support more than 1 action at at time
        # TODO: support batches for parallel environment processing

        player_id = 0
        action_name = self.action_space.action_names[0]

        if isinstance(action, int) or np.isscalar(action):
            action_data = [action]
        elif isinstance(action, dict):

            player_id = action['player']
            del action['player']

            assert len(action) == 1, "Only 1 action can be performed on each step."

            action_name = next(iter(action))
            action_data = action[action_name]
        elif isinstance(action, list) or isinstance(action, np.ndarray):
            action_data = action

        reward, done, info = self._players[player_id].step(action_name, action_data)
        self._last_observation[player_id] = np.array(self._players[player_id].observe(), copy=False)
        return self._last_observation[player_id], reward, done, info

    def reset(self, level_id=None, level_string=None):

        if level_string is not None:
            self._grid.load_level_string(level_string)
        elif level_id is not None:
            self._grid.load_level(level_id)

        self.game.reset()
        player_observation = np.array(self._players[0].observe(), copy=False)
        global_observation = np.array(self.game.observe(), copy=False)

        self._last_observation[0] = player_observation

        self.player_observation_shape = player_observation.shape
        self.global_observation_shape = global_observation.shape

        self._observation_shape = player_observation.shape
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=self._observation_shape, dtype=np.uint8)

        self.action_space = GriddlyActionSpace(self._grid)
        return self._last_observation[0]

    def render(self, mode='human', observer=0):

        if observer == 'global':
            observation = np.array(self.game.observe(), copy=False)
        else:
            observation = self._last_observation[observer]

        if mode == 'human':
            if self._renderWindow.get(observer) is None:
                from griddly.RenderTools import RenderWindow
                self._renderWindow[observer] = RenderWindow(observation.shape[1], observation.shape[2])
            self._renderWindow[observer].render(observation)

        return np.array(observation, copy=False).swapaxes(0, 2)

    def get_keys_to_action(self):
        keymap = {
            (ord('a'),): 1,
            (ord('w'),): 2,
            (ord('d'),): 3,
            (ord('s'),): 4,
            (ord('e'),): 5
        }

        return keymap


class GymWrapperFactory():

    def build_gym_from_yaml(self, environment_name, yaml_file, global_observer_type=gd.ObserverType.SPRITE_2D,
                            player_observer_type=gd.ObserverType.SPRITE_2D, level=None, tile_size=None):
        register(
            id=f'GDY-{environment_name}-v0',
            entry_point='griddly:GymWrapper',
            kwargs={
                'yaml_file': yaml_file,
                'level': level,
                'tile_size': tile_size,
                'global_observer_type': global_observer_type,
                'player_observer_type': player_observer_type
            }
        )
