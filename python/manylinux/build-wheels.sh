#!/bin/bash

set -e
set -o pipefail


# Download vulkan libs
yum install -y ninja-build

export CONAN_SYSREQUIRES_SUDO=0

# Get Dependencies
/opt/python/$PYBIN/bin/pip install cmake setuptools wheel twine conan 

export PATH=$PATH:/opt/python/$PYBIN/bin

# conan install /opt/Griddly/deps --build=shaderc

echo "Conan Build Finished"

# # Cmake Build Griddlycmake . -DCMAKE_BUILD_TYPE=Release -DPython_ROOT_DIR:STRING=$(py.pythonLocation)
cd /opt/Griddly
/opt/python/$PYBIN/bin/cmake . -GNinja -DCMAKE_BUILD_TYPE=Release -DPython_ROOT_DIR:STRING=/opt/python/$PYBIN/ -DPYTHON_EXECUTABLE:FILEPATH=/opt/python/$PYBIN/bin/python -S /opt/Griddly -B /opt/Griddly/build_manylinux
/opt/python/$PYBIN/bin/cmake --build /opt/Griddly/build_manylinux --target python_griddly

# # # Create Wheel
cd python
/opt/python/$PYBIN/bin/python setup.py bdist_wheel --plat $PLAT

