import os
import logging
from collections import defaultdict, OrderedDict
from pathlib import Path
import textwrap
import yaml
import numpy as np

from griddly import GriddlyLoader, gd
from griddly.RenderTools import RenderToFile, RenderWindow
from griddly.util.breakdown import EnvironmentBreakdown


class GamesToSphix():

    def __init__(self, docs_root):
        logging.basicConfig(level=logging.DEBUG)
        self._logger = logging.getLogger('Game Doc Generator')

        self._docs_root = docs_root.resolve()

        self._env_names = set()

        self._game_documentation = defaultdict(lambda: defaultdict(dict))

        # {
        #     'doc': '',
        #     'taster': '',
        #     'images': []
        # }

    def _generate_object_description(self, game_description, yaml_string, doc_path):

        sphinx_string = ''

        tile_images, supported_observer_types = self._generate_object_tile_images(game_description, yaml_string,
                                                                                  doc_path)

        key_table_name_header = '   * - Name ->\n'
        key_table_mapchar_header = '   * - Map Char ->\n'
        key_table_render_row = defaultdict(lambda: '')

        sphinx_string += '.. list-table:: Tiles\n   :header-rows: 2\n\n'

        for object in game_description['Objects']:
            name = object['Name']
            map_character = object['MapCharacter'] if 'MapCharacter' in object else None

            if map_character is not None:
                key_table_name_header += f'     - {name}\n'
                key_table_mapchar_header += f'     - {map_character}\n'
                for observer_type in supported_observer_types:
                    observer_type_string = self._get_observer_yaml_key(observer_type)
                    key_table_render_row[
                        observer_type_string] += f'     - .. image:: {tile_images[observer_type_string][name]}\n'

        sphinx_string += key_table_name_header
        sphinx_string += key_table_mapchar_header
        for observer_type in supported_observer_types:
            observer_type_string = self._get_observer_yaml_key(observer_type)
            sphinx_string += f'   * - {observer_type_string}\n{key_table_render_row[observer_type_string]}'

        sphinx_string += '\n\n'
        return sphinx_string

    def _generate_levels_description(self, game_breakdown):

        game_name = game_breakdown.name

        levels = game_breakdown.levels
        supported_observer_types = game_breakdown.supported_observers

        sphinx_string = ''

        level_table_header = '.. list-table:: Levels\n   :header-rows: 1\n\n'
        level_table_header += '   * - \n'
        for observer_name, observer_type in supported_observer_types.items():
            level_table_header += f'     - {observer_name}\n'

        level_table_string = ''

        level_images = {}
        for level_id, level_data in levels.items():
            level_size = level_data['Size']
            level_size_string = f'{level_size[0]}x{level_size[1]}'
            level_table_string += f'   * - .. list-table:: \n\n' \
                                  f'          * - Level ID\n' \
                                  f'            - {level_id}\n' \
                                  f'          * - Size\n' \
                                  f'            - {level_size_string}\n'

            observer_rendered_levels = level_data['Observers']
            for observer_name, np_level_image in observer_rendered_levels.items():
                relative_image_path = os.path.join('img',
                                                   f'{game_name.replace(" ", "_")}-level-{observer_name}-{level_id}.png')

                level_images[relative_image_path] = np_level_image
                level_table_string += f'     - .. thumbnail:: {relative_image_path}\n'

        level_table_string += '\n'

        sphinx_string += level_table_header
        sphinx_string += level_table_string

        return sphinx_string, level_images

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

    def _generate_game_doc(self, game_breakdown):

        description = game_breakdown.description
        name = game_breakdown.name

        images = {}
        sphinx_string = name + '\n'
        sphinx_string += '=' * len(name) + '\n\n'

        sphinx_string += 'Description\n'
        sphinx_string += '-------------\n\n'

        sphinx_string += f'{description}\n\n'

        sphinx_string += 'Levels\n'
        sphinx_string += '---------\n\n'

        level_sphinx_string, level_images = self._generate_levels_description(game_breakdown)
        images.update(level_images)
        sphinx_string += level_sphinx_string
        #
        # sphinx_string += 'Code Example\n'
        # sphinx_string += '------------\n\n'

        #sphinx_string += self._generate_code_example(game_breakdown)
        #
        # sphinx_string += 'Objects\n'
        # sphinx_string += '-------\n\n'
        #
        # sphinx_string += self._generate_object_description(game_breakdown, doc_path)
        #
        # sphinx_string += 'Actions\n'
        # sphinx_string += '-------\n\n'
        #
        # sphinx_string += self._generate_actions_description(full_gdy_path)

        sphinx_string += 'YAML\n'
        sphinx_string += '----\n\n'
        sphinx_string += '.. code-block:: YAML\n\n'
        sphinx_string += f'{textwrap.indent(game_breakdown.gdy_string, "   ")}\n\n'

        return {
            'sphinx': sphinx_string,
            'images': images
        }

    def _generate_game_taster(self, game_breakdown):
        return {
            'sphinx': game_breakdown.name,
            'images': []
        }

    def add_game(self, category, subcategory, gdy_file):

        game_breakdown = EnvironmentBreakdown(gdy_file)

        self._logger.debug(f'Game description loaded: {game_breakdown.name}')

        if game_breakdown.name not in self._env_names:
            self._env_names.add(game_breakdown.name)
        else:
            raise NameError("Cannot have GDY games with the same names")

        game_docs = {}

        self._logger.debug(f'Generating game taster for: {game_breakdown.name}')
        game_docs['taster'] = self._generate_game_taster(game_breakdown)

        self._logger.debug(f'Generating game doc for: {game_breakdown.name}')
        game_docs['doc'] = self._generate_game_doc(game_breakdown)

        self._game_documentation[category][subcategory] = game_docs

        # generateme_doc_filename = f'{doc_path}/{name}.rst'
        # with open(generated_game_doc_filename, 'w') as f:
        #     f.write(sd_gaphinx_string)
        #
        # return generated_game_doc_filename

    def generate(self):

        # generate all the images
        renderer = RenderToFile()

    # def generate(self, gdy_directory, doc_path, directories, filenames):
    #     index_sphinx_string = ''
    #
    #     doc_fullpath = os.path.realpath(f'../../../docs/games{doc_directory}')
    #
    #     index_sphinx_string += f'.. _doc_{title.lower()}:\n\n'
    #     index_sphinx_string += f'{title}\n'
    #     index_sphinx_string += '=' * len(title) + '\n\n'
    #
    #     index_sphinx_string += '.. toctree:: \n   :maxdepth: 1\n\n'
    #
    #     doc_path = Path(doc_fullpath)
    #     doc_path.mkdir(parents=True, exist_ok=True)
    #
    #     if len(filenames) > 0:
    #         img_path = Path(f'{doc_fullpath}/img')
    #         img_path.mkdir(parents=True, exist_ok=True)
    #         for filename in filenames:
    #             if filename.endswith('.yaml'):
    #                 doc_filename = self._generate_game_docs(gdy_directory, doc_fullpath, title, gdy_file=filename)
    #                 index_sphinx_string += f'   {doc_filename.replace(f"{doc_fullpath}/", "")}\n'
    #             else:
    #                 self._logger.warning(f'Ignoring file {filename} as it does not end in .yaml')
    #
    #     for dir in directories:
    #         index_sphinx_string += f'   {dir}/index.rst\n'
    #
    #     with open(f'{doc_fullpath}/index.rst', 'w') as f:
    #         f.write(index_sphinx_string)

    def _get_observer_yaml_key(self, observer_type):
        if observer_type is gd.ObserverType.SPRITE_2D:
            return "Sprite2D"
        elif observer_type is gd.ObserverType.BLOCK_2D:
            return "Block2D"
        elif observer_type is gd.ObserverType.ISOMETRIC:
            return "Isometric"
        else:
            return "Unknown"


if __name__ == '__main__':
    games_path = Path('../../../resources/games')

    docs_root = Path('../../../docs/games')
    generator = GamesToSphix(docs_root)
    for directory_dir, directory_names, filenames in os.walk(games_path):
        directory_path = Path(directory_dir)
        if directory_path == games_path:
            continue

        gdy_subdirectory = directory_path.relative_to(games_path)

        for filename in filenames:
            if filename.endswith('.yaml'):
                category, subcategory = gdy_subdirectory.parts
                gdy_file = directory_path.joinpath(filename).resolve()
                generator.add_game(category, subcategory, gdy_file)

        generator.generate()


