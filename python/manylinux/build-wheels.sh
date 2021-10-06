#!/bin/bash

set -e
set -o pipefail

# Download vulkan libs
yum install -y vulkan vulkan-devel ninja-build

# Get Dependencies
/opt/python/$PYBIN/bin/pip install cmake setuptools wheel twine

# Download and build glslc for manylinux
# VULKAN SDK does not support manylinux so cannot use the glslc version from the sdk
# https://vulkan.lunarg.com/issue/home?limit=10;q=;mine=false;org=false;khronos=false;lunarg=false;indie=false;status=new,open
cd /opt/
git clone https://github.com/google/shaderc
cd shaderc/
/opt/python/$PYBIN/bin/python ./utils/git-sync-deps
/opt/python/$PYBIN/bin/cmake . -GNinja -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE:FILEPATH=/opt/python/$PYBIN/bin/python
/opt/python/$PYBIN/bin/cmake --build . --target glslc_exe
ln -s /opt/shaderc/glslc/glslc /usr/bin/glslc

# Cmake Build Griddly
cd /opt/Griddly
/opt/python/$PYBIN/bin/cmake -E make_directory build
cd build
/opt/python/$PYBIN/bin/cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE:FILEPATH=/opt/python/$PYBIN/bin/python -DPYBIND11_PYTHON_VERSION=$PYVERSION
/opt/python/$PYBIN/bin/cmake --build . --target pygriddly

# Create Wheel
cd python
/opt/python/$PYBIN/bin/python setup.py bdist_wheel --plat $PLAT

