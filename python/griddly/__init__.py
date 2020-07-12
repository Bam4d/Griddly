import os
import sys
import importlib
import yaml

# The python libs are found in the current directory
module_path = os.path.dirname(os.path.realpath(__file__))
libs_path = os.path.join(module_path, 'libs')

sys.path.extend([libs_path])

debug_path = os.path.join(module_path, '../../Debug/bin')
sys.path.extend([debug_path])

# Load the binary
gd = importlib.import_module('python_griddly')


class GriddlyLoader():
    def __init__(self, gdy_path=None, image_path=None, shader_path=None):
        module_path = os.path.dirname(os.path.realpath(__file__))
        self._image_path = os.path.join(module_path, 'resources', 'images') if image_path is None else image_path
        self._shader_path = os.path.join(module_path, 'resources', 'shaders') if shader_path is None else shader_path
        self._gdy_path = os.path.join(module_path, 'resources', 'games') if gdy_path is None else gdy_path
        self._gdy_reader = gd.GDYReader(self._image_path, self._shader_path)

    def get_full_path(self, gdy_path):
        # Assume the file is relative first and if not, try to find it in the pre-defined games
        fullpath = gdy_path if os.path.exists(gdy_path) else os.path.join(self._gdy_path, gdy_path)
        # (for debugging only) look in parent directory resources because we might not have built the latest version
        fullpath = fullpath if os.path.exists(fullpath) else os.path.realpath(
            os.path.join(self._gdy_path + '../../../../../resources/games', gdy_path))
        return fullpath

    def load_game(self, gdy_path):
        return self._gdy_reader.load(self.get_full_path(gdy_path))

    def load_gdy(self, gdy_path):
        with open(self.get_full_path(gdy_path)) as gdy_file:
            return yaml.load(gdy_file)


from griddly.GymWrapper import GymWrapper, GymWrapperFactory
