import yaml
import numpy as np
from griddly import GriddlyLoader, gd
from griddly.util.vector_visualization import Vector2RGB


class TemporaryEnvironment:
    """
    Because we have to load the game many different times with different configurations, this class makes sure we clean up objects we dont need
    """

    def __init__(self, loader, gdy_string, observer_name):
        self.gdy = loader.load_string(gdy_string)
        self.observer_type = self._get_observer_type(observer_name)
        self.observer_name = observer_name

    def __enter__(self):
        self.game = self.gdy.create_game(self.observer_name)
        self.players = []
        for p in range(self.gdy.get_player_count()):
            self.players.append(self.game.register_player(f"P{p}", self.observer_name))
        self.game.init(False)

        return self

    def __exit__(self, type, value, traceback):
        self.game.release()

    def _get_observer_type(self, observer_type_or_string):
        if isinstance(observer_type_or_string, gd.ObserverType):
            return observer_type_or_string
        else:
            return self.gdy.get_observer_type(observer_type_or_string)

    def render_rgb(self):

        # Set copy to true here because we want to use these later after environment resets
        observation = np.array(self.game.observe(), copy=True)

        if self.observer_type == gd.ObserverType.VECTOR:
            self._vector2rgb = Vector2RGB(10, len(self.game.get_object_names()))
            return self._vector2rgb.convert(observation)
        else:
            return observation


class EnvironmentBreakdown:
    def __init__(self, gdy_filename):
        """
        Creates a breakdown of the and.
        """

        with open(gdy_filename, "r") as gdy_file:
            self.gdy_string = gdy_file.read()

        self.gdy_resource_path = gdy_filename

        self.gdy = yaml.load(self.gdy_string, Loader=yaml.SafeLoader)

        self.loader = GriddlyLoader()
        self.objects = {}
        self.levels = {}

        self._populate_objects()
        self._populate_common_properties()

        self._populate_tiles()
        self._populate_levels()

    def _env(self, observer_name):
        return TemporaryEnvironment(self.loader, self.gdy_string, observer_name)

    def _populate_objects(self):
        for object in self.gdy["Objects"]:
            self.objects[object["Name"]] = {
                "MapCharacter": object.get("MapCharacter", None),
                "Tiles": {},
            }

    def _populate_common_properties(self):

        self.environment = self.gdy["Environment"]
        self.name = self.environment["Name"]
        self.description = self.environment.get("Description", "")

        self.player_count = 1

        # observer types
        self.supported_observers = {"Vector"}

        # Look through objects
        objects = self.gdy["Objects"]
        for object in objects:
            for observer_name, config in object["Observers"].items():
                self.supported_observers.add(observer_name)

        self.observer_configs = {"Block2D": {}, "Sprite2D": {}, "Vector": {}, "Isometric": {}}
        if "Observers" in self.environment:
            for observer_name, config in self.environment["Observers"].items():
                self.observer_configs[observer_name] = config


        with self._env("Vector") as env:
            self.player_count = env.gdy.get_player_count()
            self.action_mappings = env.gdy.get_action_input_mappings()

        self.has_avatar = "AvatarObject" in self.gdy["Environment"]["Player"]

    def _populate_tiles(self):

        # Create a level thats just a list of all the map characters
        all_tiles = []
        ordered_object_names = []
        for object_name, object in self.objects.items():
            if object["MapCharacter"] is not None:
                all_tiles.append(object["MapCharacter"])
                ordered_object_names.append(object_name)

        join_string = "."

        all_tiles_string = join_string.join(all_tiles) + join_string

        for observer_name in self.supported_observers:

            if "TrackAvatar" in self.observer_configs[observer_name]:
                if self.observer_configs[observer_name]["TrackAvatar"]:
                    continue

            with self._env(observer_name) as env:
                env.game.load_level_string(f"{all_tiles_string}\n")
                env.game.reset()
                rendered_sprite_map = env.render_rgb()

                tile_size = env.game.get_tile_size()
                for i, object_name in enumerate(ordered_object_names):

                    if env.observer_type == gd.ObserverType.VECTOR:

                        tile_size = [10, 10]
                        tile_pos_x = 2 * i * tile_size[0]
                        tile_width = tile_size[0]
                        tile_pos_y = 0
                        tile_height = tile_size[1]

                    elif env.observer_type == gd.ObserverType.ISOMETRIC:

                        iso_tile_height = int(
                            self.gdy["Environment"]["Observers"]["Isometric"][
                                "IsoTileHeight"
                            ]
                        )

                        tile_pos_x = i * int(tile_size[0])
                        tile_width = tile_size[0]
                        tile_pos_y = i * iso_tile_height
                        tile_height = tile_size[1]

                    else:

                        tile_pos_x = 2 * i * tile_size[0]
                        tile_width = tile_size[0]
                        tile_pos_y = 0
                        tile_height = tile_size[1]

                    tile_image = rendered_sprite_map[
                        :,
                        tile_pos_x : tile_pos_x + tile_width,
                        tile_pos_y : tile_pos_y + tile_height,
                    ]

                    self.objects[object_name]["Tiles"][observer_name] = {
                        "Image": tile_image,
                        "Size": tile_size,
                    }

    def _populate_levels(self):

        for i, level in enumerate(self.gdy["Environment"]["Levels"]):
            self.levels[i] = {"Map": level, "Observers": {}}

        for observer_name in self.supported_observers:
            with self._env(observer_name) as env:

                if "TrackAvatar" in self.observer_configs[observer_name]:
                    if self.observer_configs[observer_name]["TrackAvatar"]:
                        continue

                for l, level in self.levels.items():
                    env.game.load_level(l)
                    env.game.reset()
                    rendered_level = env.render_rgb()
                    self.levels[l]["Observers"][observer_name] = rendered_level
                    self.levels[l]["Size"] = [
                        env.game.get_width(),
                        env.game.get_height(),
                    ]
