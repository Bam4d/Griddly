#!/bin/bash

if [ ! -d "../build_wasm" ]; then
    mkdir ../build_wasm
fi

if [ ! -d "jiddly-app/src/wasm/" ]; then
    mkdir jiddly-app/src/wasm/
fi

if [ ! -d "jiddly-app/public/js" ]; then
    mkdir jiddly-app/public/js/
fi

rm jiddly-app/src/wasm/jiddly.js ../bin/jiddly.js
rm jiddly-app/public/js/jiddly.wasm ../bin/jiddly.wasm 

pushd ../build_wasm
    emcmake cmake ..
    emmake make
popd

cp ../bin/jiddly.js jiddly-app/src/wasm/jiddly.js
cp ../bin/jiddly.wasm jiddly-app/public/js/jiddly.wasm

cp -R ../resources/games jiddly-app/public/resources/games
cp -R ../resources/images jiddly-app/public/resources/images
cp ../resources/gdy-schema.json jiddly-app/public/resources/gdy-schema.json