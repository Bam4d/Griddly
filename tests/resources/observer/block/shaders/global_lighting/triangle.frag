#version 460

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec4 inLighting;

layout (location = 0) out vec4 outFragColor;

void main() 
{
  outFragColor = inColor * inLighting;
}