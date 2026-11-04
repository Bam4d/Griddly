#!/bin/bash

BUILD_TYPE=$1

if [ ! -d "../build_wasm" ]; then
    mkdir ../build_wasm
fi

if [ -z "$1" ]; then
   BUILD_TYPE="Debug"
fi

rm -f griddlyjs-app/src/wasm/griddlyjs.js ../build_wasm/bin/griddlyjs.js
rm -f griddlyjs-app/public/js/griddlyjs.wasm ../build_wasm/bin/griddlyjs.wasm ../build_wasm/bin/griddlyjs.data

EMSDK=${EMSDK:-/tmp/emsdk}
source "$EMSDK/emsdk_env.sh" > /dev/null

pushd ../build_wasm
    emcmake cmake .. -DWASM=ON -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -B .
    emcmake cmake --build . -j "$(nproc)"
popd

cp ../build_wasm/bin/griddlyjs.js griddlyjs-app/src/wasm/griddlyjs.js
cp ../build_wasm/bin/griddlyjs.wasm griddlyjs-app/public/js/griddlyjs.wasm
cp ../build_wasm/bin/griddlyjs.data griddlyjs-app/public/js/griddlyjs.data 2>/dev/null || true
