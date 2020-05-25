import os
import sys
import importlib

# The python libs are found in the current directory
module_path = os.path.dirname(os.path.realpath(__file__))
libs_path = os.path.join(module_path, 'libs')
resources_path = os.path.join(module_path, 'resources')
sys.path.extend([libs_path])

# Load the binary
gd = importlib.import_module('python_griddly')

# Helper method for loading levels
def griddly_loader(resources_path=resources_path):
    return GriddlyLoader(resources_path)

class GriddlyLoader():
    def __init__(self, resources_path):
        self._resources_path = resources_path
        self._gdy_reader = gd.GDYReader(resources_path)

    def load_game_description(self, path):
        return self._gdy_reader.load(os.path.join(self._resources_path, 'games', path))

from griddly_python.GymWrapper import GymWrapper, GymWrapperFactory
