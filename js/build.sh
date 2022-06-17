#!/bin/bash

BUILD_TYPE=$1

if [ -z "$1" ]; then
   BUILD_TYPE="Debug"
fi


if [ ! -d "../build_wasm" ]; then
    mkdir ../build_wasm
fi

if [ ! -d "jiddly-app/src/wasm/" ]; then
    mkdir jiddly-app/src/wasm/
fi

if [ ! -d "jiddly-app/public/js" ]; then
    mkdir jiddly-app/public/js/
fi

if [ ! -d "jiddly-app/public/resources" ]; then
    mkdir jiddly-app/public/resources/
fi

rm jiddly-app/src/wasm/jiddly.js ../bin/jiddly.js
rm jiddly-app/public/js/jiddly.wasm ../bin/jiddly.wasm 

pushd ../build_wasm
    emcmake cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE -B .
    emmake make
popd

cp ../$BUILD_TYPE/bin/jiddly.js jiddly-app/src/wasm/jiddly.js
cp ../$BUILD_TYPE/bin/jiddly.wasm jiddly-app/public/js/jiddly.wasm

cp -R ../resources/games jiddly-app/public/resources/games
cp -R ../resources/images jiddly-app/public/resources/images
cp ../resources/gdy-schema.json jiddly-app/public/resources/gdy-schema.json
