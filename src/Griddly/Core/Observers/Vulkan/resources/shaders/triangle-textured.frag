#version 450

layout(binding=0)uniform sampler2DArray samplerArray;

layout(location=0)in vec4 inColor;
layout(location=1)in vec3 inFragTextureCoords;
layout(location=2)flat in int isIsOutline;

layout(location=0)out vec4 outFragColor;

void main()
{
  if(isIsOutline==1){
    // Just multiply by the alpha channel of the object
    outFragColor=inColor*texture(samplerArray,inFragTextureCoords).w;
  }else{
    outFragColor=texture(samplerArray,inFragTextureCoords);
  }
  
}