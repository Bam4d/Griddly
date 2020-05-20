import python_griddle as gd
import numpy as np
from gym.envs.registration import register
import gym

from tools import RenderWindow


class GymWrapper(gym.Env):

    def __init__(self, yaml_file, level=0, render_mode=gd.ObserverType.SPRITE_2D):
        gdy = gd.GDYReader()

        gdy_description = gdy.load(yaml_file)

        self._grid = gdy_description.load_level(level)
        self.game = self._grid.create_game(render_mode)
        self._player = self.game.add_player('Player 1', render_mode)
        self.game.init()

    def step(self, action):
        reward, done = self._player.step('move', [action])
        self._last_observation = np.array(self._player.observe(), copy=False).swapaxes(0,2)
        return self._last_observation, reward, done, None

    def reset(self):

        self._last_observation = np.array(self.game.reset(), copy=False).swapaxes(0,2)

        width = self._grid .get_width()
        height = self._grid .get_height()
        self._renderWindow = RenderWindow(32*width, 32*height)

        observation_shape = [height, width, 3]
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=observation_shape, dtype=np.uint8)

    def render(self, mode='human'):
        if mode == 'human':
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

    def build_gym_from_yaml(self, environment_name, yaml_file, render_mode=gd.ObserverType.SPRITE_2D, level=None):

        register(
            id=f'GDY-{environment_name}-v0',
            entry_point='tools:GymWrapper',
            kwargs={
                'yaml_file': yaml_file,
                'level': level,
                'render_mode': render_mode
            }
        )



