#!/bin/bash

if [ ! -d "../build_wasm" ]; then
    mkdir ../build_wasm
fi

rm html/jiddly.js ../bin/jiddly.js
rm html/jiddly.wasm ../bin/jiddly.wasm 

pushd ../build_wasm
    emcmake cmake ..
    emmake make
popd

cp ../bin/jiddly.js html/jiddly.js
cp ../bin/jiddly.wasm html/jiddly.wasm