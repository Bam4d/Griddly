#!/bin/bash
# Builds manylinux wheels for one or more python versions.
# Usage: PYBIN=cp312-cp312 PLAT=manylinux_2_28_x86_64 ./manylinux-build.sh
#        or: ./manylinux-build.sh cp312-cp312 cp313-cp313   (defaults to manylinux_2_28_x86_64)
set -ex

GRIDDLY_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PLAT="${PLAT:-manylinux_2_28_x86_64}"
IMAGE="quay.io/pypa/${PLAT}"

if [ $# -gt 0 ]; then
  PYBINS="$@"
else
  PYBINS="${PYBIN:-cp312-cp312}"
fi

for PYBIN in $PYBINS; do
  echo "Building wheel for $PYBIN ($PLAT)"
  docker run --rm -e PYBIN="$PYBIN" -e PLAT="$PLAT" \
    -v "$GRIDDLY_ROOT:/opt/Griddly" \
    -v griddly-conan-cache:/root/.conan2 \
    "$IMAGE" /opt/Griddly/python/manylinux/build-wheels.sh
done
