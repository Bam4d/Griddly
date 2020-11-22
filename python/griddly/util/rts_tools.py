import gym
import numpy as np
from griddly.util.vector_visualization import Vector2RGB

from griddly import GymWrapper


class ValidatedMultiDiscrete(gym.spaces.MultiDiscrete):
    """
    The same action space as MultiDiscrete, however sampling this action space only results in valid actions
    """

    def __init__(self, nvec, masking_wrapper):
        self._masking_wrapper = masking_wrapper
        super().__init__(nvec)

    def sample(self, player_id=None):
        if player_id is None:
            player_id = np.random.choice(self._masking_wrapper.player_count)

        # Sample a location with valid actions
        available_actions = [a for a in self._masking_wrapper.env.game.get_available_actions(player_id+1).items()]
        available_actions_choice = np.random.choice(len(available_actions))
        location, actions = available_actions[available_actions_choice]

        available_action_ids = [aid for aid in self._masking_wrapper.env.game.get_available_action_ids(location, list(actions)).items()]
        available_action_ids_choice = np.random.choice(len(available_action_ids))
        action_name, action_ids = available_action_ids[available_action_ids_choice]

        action_name_idx = self._masking_wrapper.action_names.index(action_name)
        action_id = np.random.choice(action_ids)

        return [player_id, location[0], location[1], action_name_idx, action_id]


class InvalidMaskingRTSWrapper(gym.Wrapper):

    def __init__(self, env):
        super().__init__(env)

    def step(self, action):
        player_id = action[0]
        x = action[1]
        y = action[2]
        action_name = self.action_names[action[3]]
        action_parameter = action[4]

        action_data = [x, y, action_parameter]

        reward, done, info = self.env._players[player_id].step(action_name, action_data)
        self.env._last_observation[player_id] = np.array(self.env._players[player_id].observe(), copy=False)
        return self.env._last_observation[player_id], reward, done, info

    def reset(self, level_id=None, level_string=None):

        if level_string is not None:
            self.env._grid.load_level_string(level_string)
        elif level_id is not None:
            self.env._grid.load_level(level_id)

        self.env.game.reset()
        player_observation = np.array(self.env._players[0].observe(), copy=False)
        global_observation = np.array(self.env.game.observe(), copy=False)

        self.env._last_observation[0] = player_observation

        self.env.player_observation_shape = player_observation.shape
        self.env.global_observation_shape = global_observation.shape

        self._observation_shape = player_observation.shape
        self.env.observation_space = gym.spaces.Box(low=0, high=255, shape=self._observation_shape, dtype=np.uint8)

        self.env._vector2rgb = Vector2RGB(10, self._observation_shape[0])

        self.env.action_space = self._create_action_space()

        self.action_space = self.env.action_space
        self.observation_space = self.env.observation_space

        return self.env._last_observation[0]

    def get_unit_location_mask(self, player_id):
        availableaction_names = self.env.game.get_available_action_names(player_id)

    def get_unit_action_mask(self, location, action_names):
        available_actions_ids = self.env.game.get_available_action_ids(location, action_names)

    def _create_action_space(self):

        # Convert action to GriddlyActionASpace
        self.player_count = self.env._grid.get_player_count()
        self.action_input_mappings = self.env._grid.get_action_input_mappings()

        grid_width = self.env._grid.get_width()
        grid_height = self.env._grid.get_height()

        self.avatar_object = self.env._grid.get_avatar_object()

        has_avatar = self.avatar_object is not None and len(self.avatar_object) > 0

        if has_avatar:
            raise RuntimeError("Cannot use MultiDiscreteRTSWrapper with environments that control single avatars")

        self.valid_action_mappings = {}
        self.action_names = []
        max_action_ids = 0
        for action_name, mapping in sorted(self.action_input_mappings.items()):
            if not mapping['Internal']:
                self.action_names.append(action_name)
                num_action_ids = len(mapping['InputMappings'])
                if max_action_ids < num_action_ids:
                    max_action_ids = num_action_ids
                self.valid_action_mappings[action_name] = num_action_ids

        multi_discrete_space = [self.player_count, grid_width, grid_height, len(self.valid_action_mappings),
                                max_action_ids]
        return ValidatedMultiDiscrete(multi_discrete_space, self)
