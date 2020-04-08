#version 450

layout (binding = 0) uniform sampler2DArray samplerArray;

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inFragTextureCoords;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = texture(samplerArray, inFragTextureCoords) * vec4(inColor, 1.0f);
}