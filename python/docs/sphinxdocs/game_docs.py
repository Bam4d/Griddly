import os
import logging
from collections import defaultdict, OrderedDict
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

    def _generate_object_tile_images(self, objects, doc_path, game_name, gdy_file):

        # img/sokoban-wall-x.png
        tile_images = defaultdict(dict)

        # load a simple griddly env with each tile printed
        loader = GriddlyLoader()
        renderer = RenderToFile()

        level_string = ''
        for i, object in enumerate(objects):
            if 'MapCharacter' in object:
                name = object['Name']
                level_string += f'{object["MapCharacter"]} '

        for observer_type in self._observer_types:
            grid = loader.load_game(gdy_file)

            player_count = grid.get_player_count()
            tileSize = grid.get_tile_size()

            game = grid.create_game(observer_type)

            players = []
            for p in range(player_count):
                players.append(game.register_player(f'P{p}', observer_type))

            observer_type_string = self._get_observer_type_string(observer_type)

            game.init()
            grid.load_level_string(f'{level_string}\n')
            game.reset()
            rendered_sprite_map = np.array(game.observe(), copy=False)

            i = 0
            for object in objects:
                if 'MapCharacter' in object:
                    name = object['Name']
                    relative_image_path = os.path.join('img',
                                                       f'{game_name.replace(" ", "_")}-object-{observer_type_string}-{name}.png')
                    doc_image_path = os.path.join(doc_path, relative_image_path)

                    single_sprite = rendered_sprite_map[:, i * tileSize:i * tileSize + tileSize, :]
                    tile_images[observer_type_string][name] = relative_image_path
                    renderer.render(single_sprite, doc_image_path)
                    i += 1

            # We are creating loads of game instances. this forces the release of vulkan resources before the python GC
            game.release()

        return tile_images

    def _generate_object_description(self, objects, doc_path, game_name, full_gdy_path):

        sphinx_string = ''

        tile_images = self._generate_object_tile_images(objects, doc_path, game_name, full_gdy_path)

        key_table_name_header = '   * - Name ->\n'
        key_table_mapchar_header = '   * - Map Char ->\n'
        key_table_render_row = defaultdict(lambda: '')

        sphinx_string += '.. list-table:: Tiles\n   :header-rows: 2\n\n'

        for object in objects:
            name = object['Name']
            map_character = object['MapCharacter'] if 'MapCharacter' in object else None

            if map_character is not None:
                key_table_name_header += f'     - {name}\n'
                key_table_mapchar_header += f'     - {map_character}\n'
                for observer_type in self._observer_types:
                    observer_type_string = self._get_observer_type_string(observer_type)
                    key_table_render_row[
                        observer_type_string] += f'     - .. image:: {tile_images[observer_type_string][name]}\n'

        sphinx_string += key_table_name_header
        sphinx_string += key_table_mapchar_header
        for observer_type in self._observer_types:
            observer_type_string = self._get_observer_type_string(observer_type)
            sphinx_string += f'   * - {observer_type_string}\n{key_table_render_row[observer_type_string]}'

        sphinx_string += '\n\n'
        return sphinx_string

    def _generate_level_data(self, game_name, num_levels, doc_path, full_gdy_path):

        # load a simple griddly env with each tile printed
        loader = GriddlyLoader()

        renderer = RenderToFile()

        level_images = defaultdict(dict)
        level_sizes = {}

        for observer_type in self._observer_types:
            grid = loader.load_game(full_gdy_path)
            game = grid.create_game(observer_type)

            players = []
            for p in range(grid.get_player_count()):
                players.append(game.register_player(f'P{p}', observer_type))

            game.init()

            for level in range(num_levels):

                observer_type_string = self._get_observer_type_string(observer_type)

                grid.load_level(level)
                game.reset()

                rendered_level = np.array(game.observe(), copy=False)

                relative_image_path = os.path.join('img',
                                                   f'{game_name.replace(" ", "_")}-level-{observer_type_string}-{level}.png')
                doc_image_path = os.path.join(doc_path, relative_image_path)
                renderer.render(rendered_level, doc_image_path)
                level_images[observer_type_string][level] = relative_image_path
                level_sizes[level] = {'width': grid.get_width(), 'height': grid.get_height()}

            # We are creating loads of game instances. this forces the release of vulkan resources before the python GC
            game.release()

        return level_images, level_sizes

    def _generate_levels_description(self, environment, doc_path, full_gdy_path):

        game_name = environment['Name']
        num_levels = len(environment['Levels'])

        sphinx_string = ''

        level_images, level_sizes = self._generate_level_data(game_name, num_levels, doc_path, full_gdy_path)

        level_table_header = '.. list-table:: Levels\n   :header-rows: 1\n\n'
        level_table_header += '   * - \n'
        for observer_type in self._observer_types:
            observer_type_string = self._get_observer_type_string(observer_type)
            level_table_header += f'     - {observer_type_string}\n'

        level_table_string = ''
        for level in range(num_levels):
            level_size = level_sizes[level]
            level_size_string = f'{level_size["width"]}x{level_size["height"]}'
            level_table_string += f'   * - .. list-table:: \n\n' \
                                  f'          * - ID\n' \
                                  f'            - {level}\n' \
                                  f'          * - Size\n' \
                                  f'            - {level_size_string}\n'

            for observer_type in self._observer_types:
                observer_type_string = self._get_observer_type_string(observer_type)

                level_image = level_images[observer_type_string][level]
                level_table_string += f'     - .. thumbnail:: {level_image}\n'

        level_table_string += '\n'

        sphinx_string += level_table_header
        sphinx_string += level_table_string

        return sphinx_string

    def _generate_code_example(self, player_count, game_name, file_name, title):

        formatted_game_name = game_name.replace(' ', '-')
        code_example_sphinx = ''

        if player_count == 1:
            single_step_code = """
        obs, reward, done, info = env.step(env.action_space.sample())
        env.render()
"""
        elif player_count > 1:
            single_step_code = """
        for p in range(env.action_space.player_count):
            sampled_action_def = np.random.choice(env.action_space.action_names)
            sampled_action_space = env.action_space.action_space_dict[sampled_action_def].sample()

            action = {
                'player': p,
                sampled_action_def: sampled_action_space
            }
            obs, reward, done, info = env.step(action)
            
            env.render(observer=p)
"""

        basic_code_example = f"""
import gym
import numpy as np
import griddly

if __name__ == '__main__':

    env = gym.make('GDY-{formatted_game_name}-v0')
    env.reset()
    
    # Replace with your own control algorithm!
    for s in range(1000):{single_step_code}
        env.render(observer='global')
"""

        formatted_game_name_adv = f'{formatted_game_name}-Adv'

        advanced_code_example = f"""
import gym
import numpy as np
from griddly import GymWrapperFactory, gd

if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml(
        '{formatted_game_name_adv}',
        '{title}/{file_name}',
        level=0,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        player_observer_type=gd.ObserverType.SPRITE_2D,
        tile_size=10
    )

    env = gym.make('GDY-{formatted_game_name_adv}-v0')
    env.reset()

    # Replace with your own control algorithm!
    for s in range(1000):{single_step_code}
        env.render(observer='global')
"""

        code_example_sphinx += 'Basic\n'
        code_example_sphinx += '^^^^^\n\n'
        code_example_sphinx += 'The most basic way to create a Griddly Gym Environment. ' \
                               'Defaults to level 0 and SPRITE_2D rendering.\n\n'
        code_example_sphinx += f'.. code-block:: python\n\n{textwrap.indent(basic_code_example, "   ")}\n\n'

        code_example_sphinx += 'Advanced\n'
        code_example_sphinx += '^^^^^^^^\n\n'
        code_example_sphinx += 'Create a customized Griddly Gym environment using the ``GymWrapperFactory``\n\n'
        code_example_sphinx += f'.. code-block:: python\n\n{textwrap.indent(advanced_code_example, "   ")}\n\n'

        return code_example_sphinx

    def _generate_actions_description(self, full_gdy_path):

        # load a simple griddly env with each tile printed
        loader = GriddlyLoader()

        grid = loader.load_game(full_gdy_path)

        action_mappings = grid.get_action_input_mappings()

        sphinx_string = ''

        for action_name, action_details in action_mappings.items():
            sphinx_string += f'{action_name}\n'
            sphinx_string += '^' * len(action_name) + '\n\n'

            if 'Relative' in action_details and action_details['Relative']:
                sphinx_string += ':Relative: The actions are calculated relative to the object being controlled.\n\n'
            if 'Internal' in action_details and action_details['Internal']:
                sphinx_string += ':Internal: This action can only be called from other actions, not by the player.\n\n'

            sphinx_string += f'.. list-table:: \n   :header-rows: 1\n\n'
            sphinx_string += '   * - Action Id\n     - Mapping\n'
            for action_id, details in sorted(action_details['InputMappings'].items()):
                description = details['Description'] if 'Description' in details else ''
                sphinx_string += f'   * - {action_id}\n     - {description}\n'

            sphinx_string += '\n\n'

        return sphinx_string

    def _generate_game_docs(self, directory_path, doc_path, title, gdy_file):
        full_gdy_path = os.path.join(directory_path, gdy_file)

        sphinx_string = ''
        with open(full_gdy_path, 'r') as game_description_yaml:
            yaml_string = game_description_yaml.read()
            game_description = yaml.load(yaml_string)
            environment = game_description['Environment']
            game_name = environment['Name']

            description = environment['Description'] if "Description" in environment else "No Description"

            if 'Player' in environment and 'Count' in environment['Player']:
                player_count = environment['Player']['Count']
            else:
                player_count = 1

            defined_action_count = len(game_description['Actions'])

            if game_name not in self._env_names:
                self._env_names.add(game_name)
            else:
                raise NameError("Cannot have GDY games with the same names")

            self._logger.debug(f'Game description loaded: {game_name}')

            sphinx_string += game_name + '\n'
            sphinx_string += '=' * len(game_name) + '\n\n'

            sphinx_string += 'Description\n'
            sphinx_string += '-------------\n\n'

            sphinx_string += f'{description}\n\n'

            sphinx_string += 'Levels\n'
            sphinx_string += '---------\n\n'

            sphinx_string += self._generate_levels_description(environment, doc_path, full_gdy_path)

            sphinx_string += 'Code Example\n'
            sphinx_string += '------------\n\n'

            sphinx_string += self._generate_code_example(player_count, game_name, gdy_file, title)

            sphinx_string += 'Objects\n'
            sphinx_string += '-------\n\n'

            sphinx_string += self._generate_object_description(game_description['Objects'], doc_path, game_name,
                                                               full_gdy_path)

            sphinx_string += 'Actions\n'
            sphinx_string += '-------\n\n'

            sphinx_string += self._generate_actions_description(full_gdy_path)

            sphinx_string += 'YAML\n'
            sphinx_string += '----\n\n'
            sphinx_string += '.. code-block:: YAML\n\n'
            sphinx_string += f'{textwrap.indent(yaml_string, "   ")}\n\n'

        generated_game_doc_filename = f'{doc_path}/{game_name}.rst'
        with open(generated_game_doc_filename, 'w') as f:
            f.write(sphinx_string)

        return generated_game_doc_filename

    def generate(self, gdy_directory, title, doc_directory, directories, filenames):
        index_sphinx_string = ''

        doc_fullpath = os.path.realpath(f'../../../docs/games{doc_directory}')

        index_sphinx_string += f'.. _doc_{title.lower()}:\n\n'
        index_sphinx_string += f'{title}\n'
        index_sphinx_string += '=' * len(title) + '\n\n'

        index_sphinx_string += '.. toctree:: \n   :maxdepth: 1\n\n'

        doc_path = Path(doc_fullpath)
        doc_path.mkdir(parents=True, exist_ok=True)

        if len(filenames) > 0:
            img_path = Path(f'{doc_fullpath}/img')
            img_path.mkdir(parents=True, exist_ok=True)
            for filename in filenames:
                if filename.endswith('.yaml'):
                    doc_filename = self._generate_game_docs(gdy_directory, doc_fullpath, title, gdy_file=filename)
                    index_sphinx_string += f'   {doc_filename.replace(f"{doc_fullpath}/", "")}\n'
                else:
                    self._logger.warning(f'Ignoring file {filename} as it does not end in .yaml')

        for dir in directories:
            index_sphinx_string += f'   {dir}/index.rst\n'

        with open(f'{doc_fullpath}/index.rst', 'w') as f:
            f.write(index_sphinx_string)

    def _get_observer_type_string(self, observer_type):
        if observer_type is gd.ObserverType.SPRITE_2D:
            return "SPRITE_2D"
        elif observer_type is gd.ObserverType.BLOCK_2D:
            return "BLOCK_2D"
        else:
            return "Unknown"


if __name__ == '__main__':
    games_dir = os.path.realpath('../../../resources/games')
    for directory_path, directory_names, filenames in os.walk(games_dir):
        generator = GamesToSphix()
        docs_path = directory_path.replace(games_dir, '')
        title = docs_path if len(docs_path) > 0 else 'Games'

        title = title.lstrip('/')
        generator.generate(directory_path, title, docs_path, directory_names, filenames)
