from pathlib import Path
import os
from sys import platform
import shutil

from setuptools import setup
from setuptools.command.install import install
from setuptools.command.develop import develop

with open('README.md', 'r') as fh:
    long_description = fh.read()

# Force platform specific wheel build
try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel


    class bdist_wheel(_bdist_wheel):

        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            self.root_is_pure = False

except ImportError:
    bdist_wheel = None


class Develop(develop):
    def run(self):
        self.package_data = {'griddle_python': griddle_package_data('Debug')}
        develop.run(self)


class Install(install):
    def run(self):
        self.package_data = {'griddle_python': griddle_package_data('Release')}
        install.run(self)


def griddle_package_data(config='Debug'):
    this_path = os.path.dirname(os.path.realpath(__file__))
    libs_path = os.path.realpath(this_path + f'/../{config}/bin')
    resources_path = os.path.realpath(this_path + '/../resources')

    libs_to_copy = []

    if platform == 'linux' or platform == 'linux2':
        libs_to_copy.extend([
            libs_path + '/libGriddle.so',
            libs_path + '/libyaml-cpp.so.0.6.3',
            libs_path + '/python_griddle.cpython-37m-x86_64-linux-gnu.so',
        ])
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
    version="0.0.1",
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
        'install': Install,
        'bdist_wheel': bdist_wheel}
)
