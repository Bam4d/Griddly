import numpy as np
from gym.envs.registration import register
import gym

from griddly_python import griddly_loader, gd


class GymWrapper(gym.Env):

    def __init__(self, yaml_file, level=0, global_render_mode=gd.ObserverType.SPRITE_2D,
                 player_render_mode=gd.ObserverType.SPRITE_2D):
        """
        Currently only supporting a single player (player 1 as defined in the environment yaml
        :param yaml_file:
        :param level:
        :param global_render_mode: the render mode for the global renderer
        :param player_render_mode: the render mode for the players
        """

        self._renderWindow = None
        loader = griddly_loader()

        game_description = loader.load_game_description(yaml_file)

        self._grid = game_description.load_level(level)

        self._num_actions = self._grid.get_num_actions()
        self._action_mode = self._grid.get_action_mode()

        self._players = []

        self.game = self._grid.create_game(global_render_mode)
        self._players.append(self.game.add_player(f'Player 1', player_render_mode))

        self._num_players = self.game.get_num_players()
        self.game.init()

    def step(self, action):

        if isinstance(action, int):
            action = [action]
        reward, done = self._players[0].step('move', action)
        self._last_observation = np.array(self._players[0].observe(), copy=False)
        return self._last_observation, reward, done, None

    def reset(self):
        self.game.reset()
        self._last_observation = np.array(self._players[0].observe(), copy=False)

        self._grid_width = self._grid.get_width()
        self._grid_height = self._grid.get_height()

        observation_shape = self._last_observation.shape
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=observation_shape, dtype=np.uint8)

        if self._action_mode == gd.ActionMode.SELECTION:
            self.action_space = gym.spaces.MultiDiscrete([self._grid_width, self._grid_height, self._num_actions])
        elif self._action_mode == gd.ActionMode.DIRECT:
            self.action_space = gym.spaces.MultiDiscrete([self._num_actions])

    def render(self, mode='human'):
        if mode == 'human':
            if self._renderWindow is None:
                from griddly_python.RenderTools import RenderWindow
                self._renderWindow = RenderWindow(32 * self._grid_width, 32 * self._grid_height)
            self._renderWindow.render(self._last_observation)
        elif mode == 'rgb_array':
            return np.array(self._last_observation, copy=False).swapaxes(0, 2)

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

    def build_gym_from_yaml(self, environment_name, yaml_file, global_render_mode=gd.ObserverType.SPRITE_2D,
                            player_render_mode=gd.ObserverType.SPRITE_2D, level=None):
        register(
            id=f'GDY-{environment_name}-v0',
            entry_point='griddly_python:GymWrapper',
            kwargs={
                'yaml_file': yaml_file,
                'level': level,
                'global_render_mode': global_render_mode,
                'player_render_mode': player_render_mode
            }
        )
