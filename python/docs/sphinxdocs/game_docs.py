import logging
import os
import textwrap
from collections import defaultdict
from pathlib import Path

from griddly.RenderTools import RenderToFile
from griddly.util.breakdown import EnvironmentBreakdown


class GamesToSphix():

    def __init__(self, docs_root, gallery_width=3):
        logging.basicConfig(level=logging.DEBUG)
        self._logger = logging.getLogger('Game Doc Generator')

        self._docs_root = docs_root.resolve()

        self._env_names = set()

        self._game_documentation = defaultdict(list)

        self._gallery_width = gallery_width

    def _generate_object_description(self, game_description):

        name = game_description.name

        sphinx_string = ''

        key_table_name_header = '   * - Name ->\n'
        key_table_mapchar_header = '   * - Map Char ->\n'
        key_table_render_row = defaultdict(lambda: '')

        sphinx_string += '.. list-table:: Tiles\n   :header-rows: 2\n\n'

        object_images = {}
        for object_name, object_data in game_description.objects.items():
            map_character = object_data['MapCharacter']

            if map_character is None:
                continue

            key_table_name_header += f'     - {object_name}\n'
            key_table_mapchar_header += f'     - {map_character}\n'
            for observer_type_string, tile_data in object_data['Tiles'].items():
                relative_image_path = os.path.join('img',
                                                   f'{name.replace(" ", "_")}-tile-{object_name}-{observer_type_string}.png')

                object_images[relative_image_path] = tile_data["Image"]
                key_table_render_row[observer_type_string] += f'     - .. image:: {relative_image_path}\n'

        sphinx_string += key_table_name_header
        sphinx_string += key_table_mapchar_header
        for observer_type_string, key_table_data in key_table_render_row.items():
            sphinx_string += f'   * - {observer_type_string}\n{key_table_data}'

        sphinx_string += '\n\n'
        return sphinx_string, object_images

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

    def _generate_code_example(self, game_breakdown):

        name = game_breakdown.name
        player_count = game_breakdown.player_count
        has_avatar = game_breakdown.has_avatar

        formatted_game_name = name.replace(' ', '-')

        wrapper = ''
        imports = ''
        code_example_sphinx = ''

        if player_count == 1:
            single_step_code = """
        obs, reward, done, info = env.step(env.action_space.sample())
        env.render() # Renders the environment from the perspective of a single player
"""
        elif player_count > 1:

            if not has_avatar:
                wrapper = 'env = InvalidMaskingRTSWrapper(env)\n'
                imports = 'from griddly.util.wrappers import InvalidMaskingRTSWrapper\n'

            single_step_code = """
        obs, reward, done, info = env.step(env.action_space.sample())
        for p in range(env.player_count):
            env.render(observer=p) # Renders the environment from the perspective of a single player
"""

        basic_code_example = f"""
import gym
import griddly
{imports}
if __name__ == '__main__':

    env = gym.make('GDY-{formatted_game_name}-v0')
    env.reset()
    {wrapper}
    # Replace with your own control algorithm!
    for s in range(1000):{single_step_code}
        env.render(observer='global') # Renders the entire environment
"""


        code_example_sphinx += 'The most basic way to create a Griddly Gym Environment. ' \
                               'Defaults to level 0 and SPRITE_2D rendering.\n\n'
        code_example_sphinx += f'.. code-block:: python\n\n{textwrap.indent(basic_code_example, "   ")}\n\n'

        return code_example_sphinx

    def _generate_actions_description(self, game_breakdown):

        # load a simple griddly env with each tile printed
        action_mappings = game_breakdown.action_mappings

        sphinx_string = ''

        for action_name, action_details in action_mappings.items():
            sphinx_string += f'{action_name}\n'
            sphinx_string += '^' * len(action_name) + '\n\n'

            mapToGrid = 'MapToGrid' in action_details and action_details['MapToGrid']

            if 'Relative' in action_details and action_details['Relative']:
                sphinx_string += ':Relative: The actions are calculated relative to the object being controlled.\n\n'
            if 'Internal' in action_details and action_details['Internal']:
                sphinx_string += ':Internal: This action can only be called from other actions, not by the player.\n\n'
            if mapToGrid:
                sphinx_string += ':MapToGrid: This action is mapped to any grid location.\n\n'

            if not mapToGrid:
                sphinx_string += f'.. list-table:: \n   :header-rows: 1\n\n'
                sphinx_string += '   * - Action Id\n     - Mapping\n'
                for action_id, details in sorted(action_details['InputMappings'].items()):
                    description = details['Description'] if 'Description' in details else ''
                    sphinx_string += f'   * - {action_id}\n     - {description}\n'

            sphinx_string += '\n\n'

        return sphinx_string

    def _generate_game_doc(self, game_breakdown, relative_gdy_path):

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

        sphinx_string += 'Code Example\n'
        sphinx_string += '------------\n\n'

        sphinx_string += self._generate_code_example(game_breakdown)

        sphinx_string += 'Objects\n'
        sphinx_string += '-------\n\n'

        object_sphinx_string, object_images = self._generate_object_description(game_breakdown)
        images.update(object_images)
        sphinx_string += object_sphinx_string

        sphinx_string += 'Actions\n'
        sphinx_string += '-------\n\n'

        sphinx_string += self._generate_actions_description(game_breakdown)

        sphinx_string += 'YAML\n'
        sphinx_string += '----\n\n'
        sphinx_string += '.. code-block:: YAML\n\n'
        sphinx_string += f'{textwrap.indent(game_breakdown.gdy_string, "   ")}\n\n'

        return {
            'sphinx': sphinx_string,
            'images': images
        }

    def _generate_taster_image(self, game_breakdown):

        name = game_breakdown.name
        levels = game_breakdown.levels

        level_data = levels[len(levels) - 1]

        if 'Isometric' in level_data['Observers']:
            np_last_image = level_data['Observers']['Isometric']
        elif 'Sprite2D' in level_data['Observers']:
            np_last_image = level_data['Observers']['Sprite2D']
        elif 'Block2D' in level_data['Observers']:
            np_last_image = level_data['Observers']['Block2D']
        else:
            np_last_image = level_data['Observers']['Vector']

        relative_image_path = os.path.join('img',
                                           f'{name.replace(" ", "_")}-taster.png')
        images = {relative_image_path: np_last_image}
        sphinx_string = f'         .. image:: {relative_image_path}\n' \
                        f'            :target: __relative__doc__link__\n' \
                        f'            :width: 200\n\n'

        return sphinx_string, images

    def _generate_game_taster(self, game_breakdown):

        images = {}
        sphinx_string = f'**{game_breakdown.name}**\n\n'

        image_sphinx_string, taster_images = self._generate_taster_image(game_breakdown)
        images.update(taster_images)
        sphinx_string += image_sphinx_string

        description = textwrap.indent(game_breakdown.description, '         ')

        sphinx_string += f'{description}\n'

        return {
            'sphinx': sphinx_string,
            'images': images
        }

    def add_game(self, category, gdy_file, relative_gdy_path):

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
        game_docs['doc'] = self._generate_game_doc(game_breakdown, relative_gdy_path)

        game_docs['breakdown'] = game_breakdown

        self._game_documentation[category].append(game_docs)

    def generate(self):

        # generate all the images
        renderer = RenderToFile()

        sphinx_string = '.. _doc_games:\n\n'

        taster_img_path = self._docs_root.joinpath('img')
        taster_img_path.mkdir(parents=True, exist_ok=True)

        for category, games_in_category in self._game_documentation.items():
            sphinx_string += '*' * len(category) + '\n'
            sphinx_string += f'{category}\n'
            sphinx_string += '*' * len(category) + '\n\n'

            # Build toc tree for index of games
            sphinx_string += '.. toctree::\n'
            sphinx_string += '   :hidden:\n\n'

            for g, game_data in enumerate(games_in_category):
                game_breakdown = game_data['breakdown']
                sphinx_string += f'   {game_breakdown.name.replace(" ", "_")}/index\n'

            sphinx_string += '\n'

            # Build games gallery
            sphinx_string += '.. list-table::\n'
            sphinx_string += '   :class: game-gallery\n\n'

            remaining_cols = 0
            for g, game_data in enumerate(games_in_category):

                game_breakdown = game_data['breakdown']
                name = game_breakdown.name

                # Create doc page
                doc_data = game_data['doc']
                doc_sphinx = doc_data['sphinx']
                doc_images = doc_data['images']

                game_doc_root = Path(f'{name.replace(" ", "_")}/')

                doc_sphinx_root = self._docs_root.joinpath(game_doc_root)
                doc_image_root = doc_sphinx_root.joinpath('img')
                doc_image_root.mkdir(parents=True, exist_ok=True)

                # Save the doc images
                for doc_image_filename, np_doc_image in doc_images.items():
                    self._logger.debug(f'Writing image {doc_image_filename}')
                    renderer.render(np_doc_image, doc_sphinx_root.joinpath(doc_image_filename))

                relative_doc_path = game_doc_root.joinpath('index.rst')

                with open(self._docs_root.joinpath(relative_doc_path), 'w') as f:
                    self._logger.debug(f'Writing doc {relative_doc_path}')
                    f.write(doc_sphinx)

                taster_data = game_data['taster']
                taster_sphinx = taster_data['sphinx']
                taster_images = taster_data['images']

                taster_sphinx = taster_sphinx.replace('__relative__doc__link__',
                                                      str(relative_doc_path).replace('rst', 'html'))

                col_index = g % self._gallery_width

                if col_index == 0:
                    sphinx_string += f'   *  -  {taster_sphinx}'
                else:
                    sphinx_string += f'      -  {taster_sphinx}'
                # Save the taster images
                for taster_image_filename, np_taster_image in taster_images.items():
                    renderer.render(np_taster_image, self._docs_root.joinpath(taster_image_filename))

                remaining_cols = 2 - col_index

            # Make sure the last row has the same number of columns even if there are no games
            for _ in range(remaining_cols):
                sphinx_string += f'      -  \n'
            sphinx_string += '\n\n'

        with open(self._docs_root.joinpath('index.rst'), 'w') as f:
            f.write(sphinx_string)

if __name__ == '__main__':
    games_path = Path('../../../resources/games')

    docs_root = Path('../../../docs/games')
    generator = GamesToSphix(docs_root)
    for directory_dir, directory_names, filenames in os.walk(games_path):
        directory_path = Path(directory_dir)
        if directory_path == games_path:
            continue

        gdy_subdirectory = directory_path.relative_to(games_path)
        print(f'Directories: {gdy_subdirectory}')

        for filename in filenames:
            if filename.endswith('.yaml'):
                category = gdy_subdirectory.parts[0]
                gdy_file = directory_path.joinpath(filename).resolve()
                relative_gdy_path = gdy_file.relative_to(games_path.resolve())
                generator.add_game(category, gdy_file, relative_gdy_path)

    generator.generate()
