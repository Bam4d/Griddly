#!/bin/bash

set -ex

echo $PYVERSION
echo $PYBIN
echo $GRIDDLY_ROOT
echo $PLAT

docker run -e PYVERSION=$PYVERSION -e PYBIN=$PYBIN -e PLAT=$PLAT -v "$GRIDDLY_ROOT:/opt/Griddly" quay.io/pypa/$PLAT /opt/Griddly/python/manylinux/build-wheels.sh