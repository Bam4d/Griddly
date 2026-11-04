#!/bin/bash
# Runs inside a pypa manylinux container. Builds Griddly with conan 2 + SDL3
# and produces a repaired manylinux wheel for $PYBIN.
set -ex
set -o pipefail

# System libraries needed to build SDL3 (and its deps) from source
yum install -y ninja-build \
  libudev-devel mesa-libGL-devel mesa-libEGL-devel mesa-libGLES-devel \
  libX11-devel libXext-devel libXcursor-devel libXfixes-devel libXi-devel \
  libXrandr-devel libXxf86vm-devel libxkbcommon-devel \
  wayland-devel wayland-protocols-devel \
  pulseaudio-libs-devel alsa-lib-devel dbus-devel ibus-devel \
  vulkan-headers vulkan-loader-devel

export CONAN_SYSREQUIRES_SUDO=0

/opt/python/$PYBIN/bin/pip install cmake setuptools wheel twine "conan==2.34.0" auditwheel
export PATH=$PATH:/opt/python/$PYBIN/bin

cd /opt/Griddly

conan profile detect --force
conan install deps/conanfile.txt -pr:b=default -pr:h=default -pr:h=deps/build.profile -s build_type=Release --build=missing -c tools.system.package_manager:mode=install -of build_manylinux

cmake -S . -B build_manylinux -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=build_manylinux/conan_toolchain.cmake \
  -DENABLE_PYTHON_BINDINGS=ON \
  -DPython_ROOT_DIR=/opt/python/$PYBIN \
  -DPython_EXECUTABLE=/opt/python/$PYBIN/bin/python

cmake --build build_manylinux --target python_griddly -j "$(nproc)"

cd python
/opt/python/$PYBIN/bin/python setup.py bdist_wheel --plat "$PLAT"
/opt/python/$PYBIN/bin/auditwheel repair dist/griddly-*.whl -w dist/
