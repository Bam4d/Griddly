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
        loader = griddly_loader()

        game_description = loader.load_game_description(yaml_file)

        self._grid = game_description.load_level(level)

        self._num_players = self._grid.get_num_players()
        self._num_actions = self._grid.get_num_actions()
        self._action_mode = self._grid.get_action_mode()

        self._players = []

        self.game = self._grid.create_game(global_render_mode)
        for p in range(self._num_players):
            self._players.append(self.game.add_player(f'Player {p}', player_render_mode))
        self.game.init()

    def step(self, action):
        reward, done = self._players[0].step('move', action)
        self._last_observation = np.array(self._players[0].observe(), copy=False).swapaxes(0, 2)
        return self._last_observation, reward, done, None

    def reset(self):
        self._last_observation = np.array(self.game.reset(), copy=False).swapaxes(0, 2)

        self._width = self._grid.get_width()
        self._height = self._grid.get_height()

        observation_shape = [self._height, self._width, 3]
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=observation_shape, dtype=np.uint8)

        if self._action_mode == gd.ActionMode.SELECTION:
            self.action_space = gym.spaces.MultiDiscrete([self._width, self._height, self._num_actions])
        elif self._action_mode == gd.actionMode.DIRECT:
            self.action_space = gym.spaces.MultiDiscrete([self._num_actions])

    def render(self, mode='human'):
        if mode == 'human':
            from griddly_python.RenderTools import RenderWindow
            self._renderWindow = RenderWindow(32 * self._width, 32 * self._height)
            self._renderWindow.render(self._last_observation)
        elif mode == 'rgb_array':
            return self._last_observation

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
