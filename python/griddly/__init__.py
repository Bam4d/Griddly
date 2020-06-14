import os
import sys
import importlib

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

    def load_game_description(self, path):

        # Assume the file is relative first and if not, try to find it in the pre-defined games
        fullpath = path if os.path.exists(path) else os.path.join(self._gdy_path, path)
        return self._gdy_reader.load(fullpath)


from griddly.GymWrapper import GymWrapper, GymWrapperFactory
