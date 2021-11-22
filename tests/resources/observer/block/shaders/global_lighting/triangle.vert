#version 460

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outLighting;

out gl_PerVertex {
  vec4 gl_Position;
};

struct GlobalVariable {
  int value;
};

struct PlayerInfo {
  vec4 playerColor;
};

struct ObjectData {
  mat4 modelMatrix;
  vec4 color;
  vec2 textureMultiply;
  int textureIndex;
  int playerId;
  int zIdx;
};

layout(std140, binding =0) uniform EnvironmentData {
  mat4 projectionMatrix;
  mat4 viewMatrix;
  vec2 gridDims;
  int playerId;
  int globalVariableCount;
  int objectVariableCount;
  int highlightPlayers;
}
environmentData;

layout(std430, binding = 1) readonly buffer PlayerInfoBuffer {
  PlayerInfo variables[];
}
playerInfoBuffer;

layout(std430, binding = 2) readonly buffer ObjectDataBuffer {
  ObjectData variables[];
}
objectDataBuffer;

layout(std430, binding = 3) readonly buffer GlobalVariableBuffer {
  GlobalVariable variables[];
}
globalVariableBuffer;

layout(push_constant) uniform PushConsts {
  int idx;
}
pushConsts;

void main() {
  ObjectData object = objectDataBuffer.variables[pushConsts.idx];
  PlayerInfo objectPlayerInfo = playerInfoBuffer.variables[object.playerId - 1];

  outColor = object.color;

  mat4 mvp = environmentData.projectionMatrix * environmentData.viewMatrix * object.modelMatrix;

  float lighting = globalVariableBuffer.variables[1].value;
  float normalizedLighting = lighting/100.0f;

  outLighting = vec4(normalizedLighting, normalizedLighting, normalizedLighting, 1.0);

  gl_Position = mvp * vec4(
                          inPosition.x,
                          inPosition.y,
                          inPosition.z,
                          1.);

}