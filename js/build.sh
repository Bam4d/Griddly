#!/bin/bash

BUILD_TYPE=$1

if [ -z "$1" ]; then
   BUILD_TYPE="Debug"
fi

rm griddlyjs-app/src/wasm/griddlyjs.js ../bin/griddlyjs.js
rm griddlyjs-app/public/js/griddlyjs.wasm ../bin/griddlyjs.wasm 

pushd ../build_wasm
    emcmake cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -B .
    emmake make
popd

cp ../$BUILD_TYPE/bin/griddlyjs.js griddlyjs-app/src/wasm/griddlyjs.js
cp ../$BUILD_TYPE/bin/griddlyjs.wasm griddlyjs-app/public/js/griddlyjs.wasm
