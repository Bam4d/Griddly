import yaml
import numpy as np
from griddly import GriddlyLoader, gd
from griddly.util.vector_visualization import Vector2RGB


class TemporaryEnvironment():
    """
    Because we have to laod the game many different times with different configurations, this class makes sure we clean up objects we dont need
    """

    def __init__(self, loader, gdy_string, observer_type):
        self.gdy = loader.load_string(gdy_string)
        self.observer_type = observer_type

    def __enter__(self):
        self.game = self.gdy.create_game(self.observer_type)
        self.players = []
        for p in range(self.gdy.get_player_count()):
            self.players.append(self.game.register_player(f'P{p}', self.observer_type))
        self.game.init(False)

        return self

    def __exit__(self, type, value, traceback):
        self.game.release()

    def render_rgb(self):

        # Set copy to true here because we want to use these later after environment resets
        observation = np.array(self.game.observe(), copy=True)

        if self.observer_type == gd.ObserverType.VECTOR:
            self._vector2rgb = Vector2RGB(10, observation.shape[0])
            return self._vector2rgb.convert(observation)
        else:
            return observation


class EnvironmentBreakdown():

    def __init__(self, gdy_filename):
        """
        Creates a breakdown of the and.
        """

        with open(gdy_filename, 'r') as gdy_file:
            self.gdy_string = gdy_file.read()

        self.gdy_resource_path = gdy_filename

        self.gdy = yaml.load(self.gdy_string, Loader=yaml.SafeLoader)

        self._all_observer_types = [
            gd.ObserverType.VECTOR,
            gd.ObserverType.SPRITE_2D,
            gd.ObserverType.BLOCK_2D,
            gd.ObserverType.ISOMETRIC,
        ]

        self.loader = GriddlyLoader()
        self.objects = {}
        self.levels = {}

        self._populate_objects()
        self._populate_common_properties()

        self._populate_tiles()
        self._populate_levels()

    def _env(self, observer_type):
        return TemporaryEnvironment(self.loader, self.gdy_string, observer_type)

    def _get_observer_yaml_key(self, observer_type):
        if observer_type is gd.ObserverType.VECTOR:
            return 'Vector'
        if observer_type is gd.ObserverType.SPRITE_2D:
            return 'Sprite2D'
        elif observer_type is gd.ObserverType.BLOCK_2D:
            return 'Block2D'
        elif observer_type is gd.ObserverType.ISOMETRIC:
            return 'Isometric'
        else:
            return 'Unknown'

    def _populate_objects(self):
        for object in self.gdy['Objects']:
            self.objects[object['Name']] = {
                'MapCharacter': object.get('MapCharacter', None),
                'Tiles': {},
            }

    def _populate_common_properties(self):

        self.name = self.gdy['Environment']['Name']
        self.description = self.gdy['Environment'].get('Description', '')

        self.player_count = 1

        # observer types
        self.supported_observers = {}

        with self._env(gd.ObserverType.VECTOR) as env:
            self.player_count = env.gdy.get_player_count()
            self.action_mappings = env.gdy.get_action_input_mappings()

        self.has_avatar = 'AvatarObject' in self.gdy['Environment']['Player']


        for observer_type in self._all_observer_types:
            try:
                with self._env(observer_type) as env:
                    observer_name = self._get_observer_yaml_key(observer_type)
                    self.supported_observers[observer_name] = observer_type

            except ValueError as e:
                continue

    def _populate_tiles(self):

        # Create a level thats just a list of all the map characters
        all_tiles = []
        ordered_object_names = []
        for object_name, object in self.objects.items():
            if object['MapCharacter'] is not None:
                all_tiles.append(object["MapCharacter"])
                ordered_object_names.append(object_name)

        join_string = '.'

        all_tiles_string = join_string.join(all_tiles) + join_string

        for observer_name, observer_type in self.supported_observers.items():

            with self._env(observer_type) as env:
                env.game.load_level_string(f'{all_tiles_string}\n')
                env.game.reset()
                rendered_sprite_map = env.render_rgb()

                tile_size = env.game.get_tile_size()
                for i, object_name in enumerate(ordered_object_names):

                    if observer_type == gd.ObserverType.VECTOR:

                        tile_size = [10, 10]
                        tile_pos_x = (2 * i * tile_size[0])
                        tile_width = tile_size[0]
                        tile_pos_y = 0
                        tile_height = tile_size[1]

                    elif observer_type == gd.ObserverType.ISOMETRIC:

                        iso_tile_height = int(self.gdy['Environment']['Observers']['Isometric']['IsoTileHeight'])

                        tile_pos_x = i * int(tile_size[0])
                        tile_width = tile_size[0]
                        tile_pos_y = i * iso_tile_height
                        tile_height = tile_size[1]

                    else:

                        tile_pos_x = (2 * i * tile_size[0])
                        tile_width = tile_size[0]
                        tile_pos_y = 0
                        tile_height = tile_size[1]

                    tile_image = rendered_sprite_map[
                                 :,
                                 tile_pos_x:tile_pos_x + tile_width,
                                 tile_pos_y:tile_pos_y + tile_height
                                 ]

                    self.objects[object_name]['Tiles'][observer_name] = {
                        'Image': tile_image,
                        'Size': tile_size,
                    }

    def _populate_levels(self):

        for i, level in enumerate(self.gdy['Environment']['Levels']):
            self.levels[i] = {
                'Map': level,
                'Observers': {}
            }

        for observer_name, observer_type in self.supported_observers.items():
            with self._env(observer_type) as env:

                for l, level in self.levels.items():
                    env.game.load_level(l)
                    env.game.reset()
                    rendered_level = env.render_rgb()
                    self.levels[l]['Observers'][observer_name] = rendered_level
                    self.levels[l]['Size'] = [env.game.get_width(), env.game.get_height()]
