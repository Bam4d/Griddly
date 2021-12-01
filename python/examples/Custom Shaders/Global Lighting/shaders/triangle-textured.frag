#version 460

layout(binding = 0) uniform sampler2DArray samplerArray;

layout(location = 0) in vec4 inLightLevel;
layout(location = 1) in vec3 inFragTextureCoords;

layout(location = 0) out vec4 outFragColor;

void main() {
  outFragColor = texture(samplerArray, inFragTextureCoords) * inLightLevel;
}