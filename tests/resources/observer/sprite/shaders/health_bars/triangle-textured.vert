#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inFragTextureCoords;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outFragTextureCoords;
layout(location = 2) out vec4 outPlayerColor;

// Deprecated
layout(location = 3) out int outHighlightPlayers;
layout(location = 4) out float outNormalizedHealth;

out gl_PerVertex {
  vec4 gl_Position;
};

struct GlobalVariable {
  int value;
};

struct ObjectVariable {
  int value;
};

struct PlayerInfo {
  vec4 playerColor;
  vec4 playerObservableGrid;
};

struct ObjectData {
  mat4 modelMatrix;
  vec4 color;
  vec2 textureMultiply;
  int textureIndex;
  int objectType;
  int playerId;
  vec4 gridPosition;
};

layout(std140, binding = 1) uniform EnvironmentData {
  mat4 projectionMatrix;
  mat4 viewMatrix;
  vec2 gridDims;
  int playerCount;
  int playerId;
  int globalVariableCount;
  int objectVariableCount;
  int globalObserverAvatarMode;
  int highlightPlayers;
}
environmentData;

layout(std430, binding = 2) readonly buffer PlayerInfoBuffer {
  PlayerInfo variables[];
}
playerInfoBuffer;

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

layout(push_constant) uniform PushConsts {
  int idx;
}
pushConsts;

int getObjectVariable(in int objectIndex, in int variableIndex, in int numVariables) {
  return objectVariableBuffer.variables[objectIndex*numVariables+variableIndex].value;
}

void main() {
  ObjectData object = objectDataBuffer.variables[pushConsts.idx];

  int health = getObjectVariable(pushConsts.idx, 0, environmentData.objectVariableCount);
  int maxHealth = getObjectVariable(pushConsts.idx, 1, environmentData.objectVariableCount);

  if(object.objectType == 1 || object.objectType == 2) {
    outNormalizedHealth = float(health)/float(maxHealth);
  } else {
    outNormalizedHealth = -1.0;
  }
  
  PlayerInfo objectPlayerInfo = playerInfoBuffer.variables[object.playerId - 1];

  outFragTextureCoords = vec3(
      inFragTextureCoords.x * object.textureMultiply.x,
      inFragTextureCoords.y * object.textureMultiply.y,
      object.textureIndex);

  mat4 mvp = environmentData.projectionMatrix * environmentData.viewMatrix * object.modelMatrix;

  gl_Position = mvp * vec4(
                          inPosition.x,
                          inPosition.y,
                          inPosition.z,
                          1.);

  if (environmentData.highlightPlayers == 1) {
    if (object.playerId > 0 && object.playerId == environmentData.playerId) {
      outPlayerColor = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
      outPlayerColor = objectPlayerInfo.playerColor;
    }

    outHighlightPlayers = 1;
  } else {
    outHighlightPlayers = 0;
    outPlayerColor = vec4(0.0);
  }
}