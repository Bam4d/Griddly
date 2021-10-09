#!/bin/bash

SHADER_OUTPUT_DIR=../../../../../../../resources/shaders

if [ ! -d $SHADER_OUTPUT_DIR ]; then
  mkdir -p $SHADER_OUTPUT_DIR;
fi

glslc triangle.frag -o $SHADER_OUTPUT_DIR/triangle.frag.spv
glslc triangle.vert -o $SHADER_OUTPUT_DIR/triangle.vert.spv

glslc triangle-textured.frag -o $SHADER_OUTPUT_DIR/triangle-textured.frag.spv
glslc triangle-textured.vert -o $SHADER_OUTPUT_DIR/triangle-textured.vert.spv