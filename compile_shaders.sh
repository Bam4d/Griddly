#!/bin/bash

cd "$(dirname "$0")"

compile_shaders_in_dir () {
  echo "Compiling shaders in $1"
  $GLSLC_BIN $1/triangle-textured.frag -o $1/triangle-textured.frag.spv
  $GLSLC_BIN $1/triangle-textured.vert -o $1/triangle-textured.vert.spv
}

compile_shaders_in_dir ./resources/shaders/default/block
compile_shaders_in_dir ./resources/shaders/default/sprite
compile_shaders_in_dir ./resources/shaders/default/isometric
compile_shaders_in_dir ./tests/resources/observer/block/shaders/global_lighting
compile_shaders_in_dir ./tests/resources/observer/isometric/shaders/lighting
compile_shaders_in_dir ./tests/resources/observer/sprite/shaders/health_bars
