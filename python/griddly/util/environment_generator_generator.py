import os
import yaml

from griddly import GymWrapper


class EnvironmentGeneratorGenerator():

    def __init__(self, gdy_path=None, yaml_file=None):
        module_path = os.path.dirname(os.path.realpath(__file__))
        self._gdy_path = os.path.join(module_path, 'resources', 'games') if gdy_path is None else gdy_path
        self._input_yaml_file = self._get_full_path(yaml_file)

    def _get_full_path(self, gdy_path):
        # Assume the file is relative first and if not, try to find it in the pre-defined games
        fullpath = gdy_path if os.path.exists(gdy_path) else os.path.join(self._gdy_path, gdy_path)
        # (for debugging only) look in parent directory resources because we might not have built the latest version
        fullpath = fullpath if os.path.exists(fullpath) else os.path.realpath(
            os.path.join(self._gdy_path + '../../../../resources/games', gdy_path))
        return fullpath

    def generate_env_yaml(self):

        level_generator_gdy = {}
        with open(self._input_yaml_file, 'r') as fs:
            self._gdy = yaml.load(fs, Loader=yaml.FullLoader)

        objects = self._gdy['Objects']
        environment = self._gdy['Environment']

        # Create the placement actions
        actions = []
        for obj in objects:
            object_name = obj["Name"]
            place_action = {
                'InputMapping': {
                    'Inputs': {
                        '1': {'Description': f'Places objects of type \"{object_name}\"'}
                    }
                },
                'Name': f'place_{object_name.lower()}',
                'Behaviours': [{
                    'Src': {
                        'Object': '_empty',
                        'Commands': [
                            {'spawn': object_name}
                        ]
                    },
                    'Dst': {
                        'Object': '_empty'
                    }
                }]

            }
            actions.append(place_action)

        level_generator_gdy['Actions'] = actions

        # Copy the Objects
        level_generator_gdy['Objects'] = objects

        # Create the environment template
        level_generator_gdy['Environment'] = {
            'Name': f'{environment["Name"]} Generator',
            'Description': f'Level Generator environment for {environment["Name"]}',
            'Observers': {k: v for k,v in environment['Observers'].items() if k in ['Sprite2D', 'Isometric']}
        }

        return yaml.dump(level_generator_gdy)

    def generate_env(self, **env_kwargs):
        env_yaml = self.generate_env_yaml()

        env_args = {
            **env_kwargs,
            'yaml_string': env_yaml,
        }

        return GymWrapper(*env_args)


if __name__ == '__main__':
    yaml_file = 'Single-Player/GVGAI/spider-nest.yaml'

    egg = EnvironmentGeneratorGenerator(yaml_file=yaml_file)

    print(egg.generate_env_yaml())
