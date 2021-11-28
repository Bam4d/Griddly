#version 460

layout(binding = 0) uniform sampler2DArray samplerArray;

layout(location = 0) in float inNormalizedHealth;
layout(location = 1) in vec3 inFragTextureCoords;

layout(location = 0) out vec4 outFragColor;

void main() {
  bool isHealthBar = false;
  // Draw health bar at the top of the sprite using distance fields
  if(inNormalizedHealth>0){
      vec2 tex_dims=vec2(textureSize(samplerArray,0));
      float bar_height=1.0/tex_dims.y;
      float bar_center_x=inNormalizedHealth/2.0;

      if(distance(inFragTextureCoords.x,bar_center_x)<bar_center_x&&distance(inFragTextureCoords.y,bar_height)<bar_height){
          if(inNormalizedHealth > 0.5) {
              outFragColor=vec4(0.0,1.0,0.,1.);
          } else if(inNormalizedHealth > 0.25) {
              outFragColor=vec4(1.0,1.0,0.,1.);
          } else {
              outFragColor=vec4(1.0,0.0,0.,1.);
          }
          isHealthBar = true;
      }
  }

  if (!isHealthBar) {
      outFragColor=texture(samplerArray,inFragTextureCoords);
  }
}