#!/bin/bash

set -ex
set -o pipefail

docker run -e PYVERSION=$PYVERSION -e PYBIN=$PYBIN -e PLAT=$PLATFORM -e GTEST_FILTER=-*BlockObserverTest*:*SpriteObserverTest* -v "$GRIDDLY_ROOT:/opt/Griddly" quay.io/pypa/$PLATFORM /opt/Griddly/python/manylinux/test.sh