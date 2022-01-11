#version 460

layout(binding = 0) uniform sampler2DArray samplerArray;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inFragTextureCoords;
layout(location = 2) in vec4 playerColor;

layout(location = 0) out vec4 outFragColor;

struct GlobalVariable {
  int value;
};

struct ObjectVariable {
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
  int objectType;
  int playerId;
  int zIdx;
};

layout(std140, binding = 1) uniform EnvironmentData {
  mat4 projectionMatrix;
  mat4 viewMatrix;
  vec2 gridDims;
  int playerId;
  int globalVariableCount;
  int objectVariableCount;
  int highlightPlayers;
}
environmentData;

layout(std430, binding = 3) readonly buffer ObjectDataBuffer {
  uint size;
  ObjectData variables[];
}
objectDataBuffer;

layout(std430, binding = 4) readonly buffer GlobalVariableBuffer {
  GlobalVariable variables[];
}
globalVariableBuffer;

layout(std430, binding = 5) readonly buffer ObjectVariableBuffer {
  ObjectVariable variables[];
}
objectVariableBuffer;

int getObjectVariable(in int objectIndex, in int variableIndex, in int numVariables) {
  return objectVariableBuffer.variables[objectIndex * numVariables + variableIndex].value;
}

void main() {

  float lightLevel = 0.0;
  for (int i = 0; i < objectDataBuffer.size; i++) {
    ObjectData object = objectDataBuffer.variables[i];

    if (object.objectType == 0 || object.objectType == 1) {
      int isLight = getObjectVariable(i, 0, 1);
      if (isLight == 1) {
        mat4 mv = environmentData.viewMatrix * object.modelMatrix;
        vec4 position = mv * vec4(0, 0, 0, 1);
        float dist_to_pixel = distance(position.xy, gl_FragCoord.xy);
        lightLevel += 1.0/pow(dist_to_pixel / 30.0, 2.0) - 0.1;
      }
    }
  }

  lightLevel = max(0, min(1.0, lightLevel));

  outFragColor = texture(samplerArray, inFragTextureCoords) * vec4(lightLevel, lightLevel, lightLevel, 1.0);
}