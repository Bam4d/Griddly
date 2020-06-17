import gym
import numpy as np
from gym.envs.registration import register

from griddly import GriddlyLoader, gd


class GymWrapper(gym.Env):

    def __init__(self, yaml_file, level=0, global_observer_type=gd.ObserverType.SPRITE_2D,
                 player_observer_type=gd.ObserverType.SPRITE_2D, image_path=None, shader_path=None):
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

        game_description = loader.load_game_description(yaml_file)

        self._grid = game_description.load_level(level)

        self.defined_actions_count = self._grid.get_defined_actions_count()
        self.action_control_scheme = self._grid.get_action_control_scheme()

        self._num_actions = 6

        self._players = []
        self.player_count = self._grid.get_player_count()

        self.game = self._grid.create_game(global_observer_type)

        for p in range(1, self.player_count + 1):
            self._players.append(self.game.register_player(f'Player {p}', player_observer_type))

        self.game.init()

    def step(self, action):
        """
        Step for a particular player in the environment
        action can be in the format:

        ...actionData - if there is a single player and a single action definition, the action parameter can just be an integer

        [playerId, ...actionData] - If there is a single action definiton but multiple players

        [actionDefinitionId, ...actionData] - If there is a single player but multiple action definitions

        [playerId, actionDefinitionId, ...actionData] - if there are multiple players and multiple defined actions
        (like move, push, punch..)

        The ...actionData parameter refers to a list of integers that define the parameters of the action.
        in SELECTIVE* action control schemes this is 3 parameters [x, y, actionId]
        in DIRECT* action control schemes this is just the single actionId parameter

        :param action:
        :return:
        """

        # TODO: support more than 1 action at at time
        # TODO: support batches for parallel environment processing

        definedActionId = 0
        playerId = 0
        actionData = []

        directControl = self.action_control_scheme == gd.ActionControlScheme.DIRECT_ABSOLUTE or \
                   self.action_control_scheme == gd.ActionControlScheme.DIRECT_RELATIVE

        if isinstance(action, int):
            assert directControl, "If the control scheme is SELECTIVE, x and y coordinates must be supplied as well as an action Id"
            assert self.defined_actions_count == 1, "when there are multiple defined actions, an array of ints need to be supplied as an action"
            assert self.player_count == 1, "when there are multiple players, an array of ints need to be supplied as an action"
            actionData = [action]
            definedActionId = 0
            playerId = 0
        elif isinstance(action, list):

            if (len(action) == 2 and directControl) or (len(action) == 4 and not directControl):
                if self.defined_actions_count == 1:
                    assert self.player_count > 1, "There is only a single player and a single action definition. Action should be supplied as a single integer"
                    playerId = action[0]
                    actionData = action[1:]
                elif self.player_count == 1:
                    assert self.defined_actions_count > 1, "There is only a single player and a single action definition. Action should be supplied as a single integer"
                    definedActionId = action[0]
                    actionData = action[1:]
            elif (len(action) == 3 and directControl) or (len(action) == 5 and not directControl):
                playerId = action[0]
                definedActionId = action[1]
                actionData = action[2:]
                assert playerId < self.player_count, "Unknown player Id"
                assert definedActionId < self.defined_actions_count, "Unknown defined action Id"

        else:
            return

        action_name = self._grid.get_action_name(definedActionId)
        reward, done = self._players[playerId].step(action_name, actionData)
        self._last_observation = np.array(self._players[0].observe(), copy=False)
        return self._last_observation, reward, done, None

    def reset(self):
        self.game.reset()
        self._last_observation = np.array(self._players[0].observe(), copy=False)

        self._grid_width = self._grid.get_width()
        self._grid_height = self._grid.get_height()

        self._observation_shape = self._last_observation.shape
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=self._observation_shape, dtype=np.uint8)

        if self.action_control_scheme == gd.ActionControlScheme.SELECTION_ABSOLUTE:
            self.action_space = gym.spaces.MultiDiscrete([self._grid_width, self._grid_height, self._num_actions])
        elif self.action_control_scheme == gd.ActionControlScheme.SELECTION_RELATIVE:
            self.action_space = gym.spaces.MultiDiscrete([self._grid_width, self._grid_height, self._num_actions])
        elif self.action_control_scheme == gd.ActionControlScheme.DIRECT_ABSOLUTE:
            self.action_space = gym.spaces.MultiDiscrete([self._num_actions])
        elif self.action_control_scheme == gd.ActionControlScheme.DIRECT_RELATIVE:
            self.action_space = gym.spaces.MultiDiscrete([self._num_actions])

        return self._last_observation

    def render(self, mode='human', observer='player'):
        observation = self._last_observation
        if observer == 'global':
            observation = np.array(self.game.observe(), copy=False)

        if mode == 'human':
            if self._renderWindow.get(observer) is None:
                from griddly.RenderTools import RenderWindow
                self._renderWindow[observer] = RenderWindow(observation.shape[1], observation.shape[2])
            self._renderWindow[observer].render(observation)
        elif mode == 'rgb_array':
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
                            player_observer_type=gd.ObserverType.SPRITE_2D, level=None):
        register(
            id=f'GDY-{environment_name}-v0',
            entry_point='griddly:GymWrapper',
            kwargs={
                'yaml_file': yaml_file,
                'level': level,
                'global_observer_type': global_observer_type,
                'player_observer_type': player_observer_type
            }
        )
