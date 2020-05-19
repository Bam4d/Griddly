import os
import sys
import importlib

# The python libs are found in the current directory
module_path = os.path.dirname(os.path.realpath(__file__))
libs_path = os.path.join(module_path, 'libs')
resources_path = os.path.join(module_path, 'resources')
sys.path.extend([libs_path])

# Load the binary
gd = importlib.import_module('python_griddle')

# Helper method for loading levels
def griddle_loader(resource_dir=resources_path):
    return gd.GDYReader(resource_dir)


from griddle_python.RenderTools import RenderWindow
from griddle_python.RenderTools import RenderToFile
from griddle_python.GymWrapper import GymWrapper, GymWrapperFactory
