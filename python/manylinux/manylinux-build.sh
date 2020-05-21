#!/bin/bash

set -ex

echo $PYTHONVERSION
echo $PYVERSION
echo $PYBIN
echo $GRIDDLE_ROOT
echo $PLAT

docker run -e PYVERSION=$PYVERSION -e PYBIN=$PYBIN -e PLAT=$PLAT -v "$GRIDDLE_ROOT:/opt/Griddle" quay.io/pypa/$PLAT /opt/Griddle/python/manylinux/build-wheels.sh