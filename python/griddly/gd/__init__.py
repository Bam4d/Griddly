import os
import sys
import importlib
# The python libs are found in the current directory
module_path = os.path.dirname(os.path.realpath(__file__))
libs_path = os.path.join(module_path, "../libs")

sys.path.extend([libs_path])

debug_path = os.path.join(module_path, "../../Debug/bin")
sys.path.extend([debug_path])

# Load the binary
sys.modules["griddly.gd"] = importlib.import_module(
    "python_griddly", package="griddly.gd"
)