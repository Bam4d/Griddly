#!/bin/bash

set -ex

echo $PYVERSION
echo $PYBIN
echo $GRIDDLY_ROOT
echo $PLATFORM

docker run -e PYVERSION=$PYVERSION -e PYBIN=$PYBIN -e PLAT=$PLATFORM -v "$GRIDDLY_ROOT:/opt/Griddly" quay.io/pypa/$PLATFORM /opt/Griddly/python/manylinux/build-wheels.sh