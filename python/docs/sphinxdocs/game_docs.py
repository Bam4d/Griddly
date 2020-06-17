import os
import logging
from pathlib import Path
import textwrap
import yaml
import numpy as np

from griddly import GriddlyLoader, gd
from griddly.RenderTools import RenderToFile


class GamesToSphix():

    def __init__(self):
        logging.basicConfig(level=logging.DEBUG)
        self._logger = logging.getLogger('Game Doc Generator')
        self._env_names = set()

        self._observer_types = [
            gd.ObserverType.SPRITE_2D,
            gd.ObserverType.BLOCK_2D
        ]

    def _generate_action_description(self, actions):
        sphinx_string = ''
        return sphinx_string

    def _generate_object_tile_image(self, objects, gdy_file):

        # img/sokoban-wall-x.png
        tile_images = {}
        object_names = []
        level_string = ''
        for i, object in enumerate(objects):
            if 'MapCharacter' in object:
                name = object['Name']
                level_string += object['MapCharacter']
                object_names.append(name)

        # load a simple griddly env with each tile printed
        loader = GriddlyLoader()
        game_description = loader.load_game_description(gdy_file)
        grid = game_description.load_level_string(level_string)

        renderer = RenderToFile()
        for i, observer_type in enumerate(self._observer_types):
            game = grid.create_game(observer_type)
            game.init()
            game.reset()
            global_observation = np.array(game.observe(), copy=False)
            renderer.render(global_observation, 'test.png')

    def _generate_object_description(self, objects, gdy_file):

        sphinx_string = ''

        self._generate_object_tile_image(objects, gdy_file)

        for object in objects:
            name = object['Name']
            map_character = object['MapCharacter'] if 'MapCharacter' in object else None

            sphinx_string += f'{name}\n'
            sphinx_string += '^' * len(name) + '\n\n'

            if map_character is not None:
                sphinx_string += f':{map_character}:\n\n'

        return sphinx_string


    def _generate_game_docs(self, directory_path, doc_path, gdy_file):
        full_gdy_path  = os.path.join(directory_path, gdy_file)

        sphinx_string = ''
        with open(full_gdy_path, 'r') as game_description_yaml:
            yaml_string = game_description_yaml.read()
            game_description = yaml.load(yaml_string)
            environment = game_description['Environment']
            game_name = environment["Name"]

            description = environment["Description"] if "Description" in environment else "No Description"

            if game_name not in self._env_names:
                self._env_names.add(game_name)
            else:
                raise NameError("Cannot have GDY games with the same names")

            self._logger.debug(f'Game description loaded: {game_name}')

            sphinx_string += game_name + '\n'
            sphinx_string += '=' * len(game_name) + '\n\n'

            sphinx_string += 'Description\n'
            sphinx_string += '-------------\n\n'

            sphinx_string += 'Actions\n'
            sphinx_string += '-------\n\n'

            sphinx_string += self._generate_action_description(game_description['Actions'])

            sphinx_string += 'Objects\n'
            sphinx_string += '-------\n\n'

            sphinx_string += self._generate_object_description(game_description['Objects'], gdy_file)

            sphinx_string += 'Observers\n'
            sphinx_string += '---------\n\n'

            sphinx_string += f'{description}\n\n'

            sphinx_string += 'YAML\n'
            sphinx_string += '----\n\n'
            sphinx_string += '.. code-block:: YAML\n\n'
            sphinx_string += f'{textwrap.indent(yaml_string, "   ")}\n\n'

        with open(f'{doc_path}/{game_name}.rst', 'w') as f:
            f.write(sphinx_string)

    def generate(self, gdy_directory, doc_directory, filenames):
        print(f'{gdy_directory}, {doc_directory}, {filenames}')

        doc_fullpath = os.path.realpath(f'../../../docs/games/{doc_directory}')

        if len(filenames) > 0:
            doc_path = Path(doc_fullpath)
            doc_path.mkdir(parents=True, exist_ok=True)
            img_path = Path(f'{doc_fullpath}/img')
            img_path.mkdir(parents=True, exist_ok=True)
            for filename in filenames:
                if filename.endswith('.yaml'):
                    self._generate_game_docs(gdy_directory, doc_fullpath, gdy_file=filename)
                else:
                    self._logger.warning(f'Ignoring file {filename} as it does not end in .yaml')




if __name__ == '__main__':
    games_dir = os.path.realpath('../../../resources/games')
    for directory_path, directory_names, filenames in os.walk(games_dir):
        generator = GamesToSphix()
        docs_path = directory_path.replace(games_dir, '')
        generator.generate(directory_path, docs_path, filenames)

