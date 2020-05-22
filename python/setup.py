from pathlib import Path
import os
import glob
from sys import platform
import shutil

from setuptools import setup, Distribution
from setuptools.command.install import install
from setuptools.command.develop import develop

with open('README.md', 'r') as fh:
    long_description = fh.read()


class Develop(develop):
    def run(self):
        self.package_data = {'griddle_python': griddle_package_data('Debug')}
        develop.run(self)


# A hack to make valid platform wheels
class BinaryDistribution(Distribution):
    def is_pure(self):
        return False

    def has_ext_modules(self):
        return True


class Install(install):
    def run(self):
        self.package_data = {'griddle_python': griddle_package_data('Release')}
        install.run(self)

    # A hack to make valid platform wheels
    def finalize_options(self):
        install.finalize_options(self)
        if self.distribution.has_ext_modules():
            self.install_lib = self.install_platlib


def griddle_package_data(config='Debug'):
    this_path = os.path.dirname(os.path.realpath(__file__))
    libs_path = os.path.realpath(this_path + f'/../{config}/bin')
    resources_path = os.path.realpath(this_path + '/../resources')

    libs_to_copy = []

    if platform == 'linux' or platform == 'linux2':
        libs_to_copy.extend(glob.glob(f'{libs_path}/python_griddle*.so'))
        #libs_to_copy.append(f'{libs_path}/libGriddle.so')
        #libs_to_copy.append(f'{libs_path}/libyaml-cpp.so.0.6.3')
    if platform == 'darwin':
        libs_to_copy.extend([])
    elif platform == 'win32':
        libs_to_copy.extend([])

    # Binary files in libraries
    griddle_package_dir = os.path.realpath(this_path + '/griddle_python/libs')

    if os.path.exists(griddle_package_dir):
        shutil.rmtree(griddle_package_dir)
    os.mkdir(griddle_package_dir)

    copied_libs = [shutil.copy(lib, griddle_package_dir) for lib in libs_to_copy]

    # Resource files
    griddle_resource_dir = os.path.realpath(this_path + '/griddle_python/resources')

    if os.path.exists(griddle_resource_dir):
        shutil.rmtree(griddle_resource_dir)
    shutil.copytree(resources_path, griddle_resource_dir)
    copied_resources = [str(f) for f in Path(griddle_resource_dir).rglob('*.*')]

    copied_files = copied_libs + copied_resources

    return copied_files


setup(
    name='griddle_python',
    version="0.0.3",
    author_email="chrisbam4d@gmail.com",
    description="Griddle Python Libraries",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/bam4d/Griddle",
    packages=['griddle_python'],
    package_data={'griddle_python': griddle_package_data('Release')},
    install_requires=[
        "numpy>=1.18.0",
        "gym==0.17.2",
        "pygame>=1.9.6",
        "matplotlib>=3.2.1"
    ],
    cmdclass={
        'develop': Develop,
        'install': Install
    },
    distclass=BinaryDistribution,

)
