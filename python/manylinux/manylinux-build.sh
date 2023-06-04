#!/bin/bash
docker run -e PYVERSION=$PYVERSION -e PYBIN=$PYBIN -e PLAT=$PLATFORM -v "$GRIDDLY_ROOT:/opt/Griddly" quay.io/pypa/$PLATFORM /opt/Griddly/python/manylinux/build.sh