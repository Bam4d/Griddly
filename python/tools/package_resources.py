# A build script that copies the resources and binaries to the griddly package
import os
import shutil
import glob
from pathlib import Path
from sys import platform


def get_libs(root_path, config="Debug"):
    libs_path = Path.joinpath(root_path.parent, f"{config}/bin").resolve()

    libs_to_copy = []

    if platform == "linux" or platform == "linux2":
        libs_to_copy.extend(glob.glob(f"{libs_path}/python_griddly*.so"))
    if platform == "darwin":
        libs_to_copy.extend(glob.glob(f"{libs_path}/python_griddly*.so"))
    elif platform == "win32":
        libs_to_copy.extend(glob.glob(f"{libs_path}/python_griddly*.pyd"))

    return libs_to_copy


def build(setup_kwargs):
    root_path = Path(__file__).parent.parent.absolute()
    resources_path = Path.joinpath(root_path.parent, "resources").resolve()

    # Find the libraries
    debug_libs = get_libs(root_path, "Debug")
    release_libs = get_libs(root_path, "Release")

    if len(debug_libs) == 0 and len(release_libs) == 0:
        print(
            "No Debug or Release libraries found, please build Griddly binaries first"
        )
        exit(1)

    if len(release_libs) > 0:
        print("Copying Release libraries ...")
        libs_to_copy = release_libs
    else:
        print("Copying Debug libraries ...")
        libs_to_copy = debug_libs

    # Destination for libraries
    griddly_lib_dir = Path.joinpath(root_path, "griddly/libs")
    griddly_lib_dir.mkdir(parents=True, exist_ok=True)

    for lib in libs_to_copy:
        shutil.copy(lib, griddly_lib_dir)

    # Destination for resources
    griddly_resource_dir = Path.joinpath(root_path, "griddly/resources")
    griddly_resource_dir.mkdir(parents=True, exist_ok=True)

    shutil.copytree(resources_path, griddly_resource_dir, dirs_exist_ok=True)

if __name__ == "__main__":
    print("Packaging resources")
    build({})
