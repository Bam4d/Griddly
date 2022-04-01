#!/bin/bash

if [ ! -d "../build_wasm" ]; then
    mkdir ../build_wasm
fi

rm jiddly-app/src/wasm/jiddly.js ../bin/jiddly.js
rm jiddly-app/public/jiddly.wasm ../bin/jiddly.wasm 

pushd ../build_wasm
    emcmake cmake ..
    emmake make
popd

cp ../bin/jiddly.js jiddly-app/src/wasm/jiddly.js
cp ../bin/jiddly.wasm jiddly-app/public/jiddly.wasm