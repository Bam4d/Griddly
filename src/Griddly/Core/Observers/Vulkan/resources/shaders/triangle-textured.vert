#version 460

layout(location=0)in vec3 inPosition;
layout(location=1)in vec2 inFragTextureCoords;

layout(location=0)out vec4 outColor;
layout(location=1)out vec3 outFragTextureCoords;

out gl_PerVertex{
    vec4 gl_Position;
};

struct GlobalVariable{
    int value;
};

struct ObjectData{
    vec2 position;
    vec2 scale;
    vec2 textureMultiply;
    mat2 rotation;
    int textureIndex;
    int playerId;
    int zIdx;
};

layout(binding=1)uniform EnvironmentData{
    vec2 gridDims;
    vec2 tileSize;
    mat4 projectionMatrix;
}environmentData;

layout(std430, binding=2)readonly buffer ObjectDataBuffer{
    ObjectData variables[];
}objectDataBuffer;

layout(std430, binding=3)readonly buffer GlobalVariableBuffer{
    GlobalVariable variables[];
}globalVariableBuffer;

layout(push_constant)uniform PushConsts{
    uint idx;
}pushConsts;

mat4 translate(vec3 d) {
    return mat4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, d.x, d.y, d.z, 1.0);
}

mat4 scale(vec3 c) {
    return mat4(c.x, 0, 0, 0, 0, c.y, 0, 0, 0, 0, c.z, 0, 0, 0, 0, 1.0);
}

mat4 rotate(mat2 r) {
    mat4 rotateMt;
    rotateMt[0].x = 1.0;
    rotateMt[0].y = 0.0;
    rotateMt[0].z = 0.0;
    rotateMt[0].w = 0.0;
    rotateMt[1].x = 0.0;
    rotateMt[1].y = 1.0;
    rotateMt[1].z = 0.0;
    rotateMt[1].w = 0.0;
    rotateMt[2].x = 0.0;
    rotateMt[2].y = 0.0;
    rotateMt[2].z = 1.0;
    rotateMt[2].w = 0.0;
    rotateMt[3].x = 0.0;
    rotateMt[3].y = 0.0;
    rotateMt[3].z = 0.0;
    rotateMt[3].w = 1.0;

    return rotateMt;
}


void main()
{
    ObjectData object = objectDataBuffer.variables[pushConsts.idx];
    GlobalVariable globalVariable = globalVariableBuffer.variables[0];

    outFragTextureCoords=vec3(
    inFragTextureCoords.x * object.textureMultiply.x,
    inFragTextureCoords.y * object.textureMultiply.y,
    object.textureIndex
    );

    mat4 translateMt = translate(vec3(object.position + 0.5, 0.0));
    mat4 scaleMt = scale(vec3(environmentData.tileSize, 1.0));
    mat4 scaleTextureMt = scale(vec3(object.scale, 1.0));
    mat4 rotateMt = rotate(object.rotation);

    mat4 mvp = environmentData.projectionMatrix * scaleMt * scaleTextureMt * rotateMt * translateMt * transpose(rotateMt);

    gl_Position=mvp*vec4(
    inPosition.x,
    inPosition.y,
    inPosition.z,
    1.
    );
}