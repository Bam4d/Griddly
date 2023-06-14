#!/bin/bash

set -ex
set -o pipefail

export CONAN_SYSREQUIRES_SUDO=0

# Get Dependencies
/opt/python/$PYBIN/bin/pip install cmake conan==1.59.0

export PATH=$PATH:/opt/python/$PYBIN/bin

echo "Conan Build Finished"

# # Cmake Build Griddly
cd /opt/Griddly
conan install deps/conanfile.txt -pr:b=default -pr:h=default -pr:h=deps/build.profile -s build_type=Release --build=* -if build_manylinux
/opt/python/$PYBIN/bin/cmake . -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DPython_ROOT_DIR:STRING=/opt/python/$PYBIN -DPYTHON_EXECUTABLE:FILEPATH=/opt/python/$PYBIN/bin/python -S /opt/Griddly -B /opt/Griddly/build_manylinux
/opt/python/$PYBIN/bin/cmake --build /opt/Griddly/build_manylinux --config Release


