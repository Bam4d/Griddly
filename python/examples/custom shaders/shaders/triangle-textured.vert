#version 460

layout(location=0)in vec3 inPosition;
layout(location=1)in vec2 inFragTextureCoords;

layout(location=0)out vec4 outColor;
layout(location=1)out vec3 outFragTextureCoords;
//layout(location=2)out int outIsOutline;
//layout(location=3)out vec4 outlineColor;
layout(location=2)out float outLighting;

#define PI 3.1415926538

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
    uint textureIndex;
    uint playerId;
    uint zIdx;
};

layout(binding=1)uniform EnvironmentData{
    vec2 gridDims;
    vec2 tileSize;
    mat4 projectionMatrix;
}environmentData;

layout(std140, binding=2)readonly buffer ObjectDataBuffer{
    ObjectData variables[];
}objectDataBuffer;

layout(std140, binding=3)readonly buffer GlobalVariableBuffer{
    GlobalVariable variables[];
}globalVariableBuffer;

layout(push_constant)uniform PushConsts{
	uint idx;
}pushConsts;

mat4 translate(vec3 d)
{
	return mat4(1, 0, 0, d.x,
	            0, 1, 0, d.y,
	            0, 0, 1, d.z,
	            0, 0, 0, 1);
}

mat4 scale(vec3 c)
{
	return mat4(c.x, 0, 0, 0,
	            0, c.y, 0, 0,
	            0, 0, c.z, 0,
	            0, 0, 0, 1);
}
mat4 scale(in mat4 m, in vec3 v)
{
    mat4 res;
    res[0] = m[0] * v[0];
    res[1] = m[1] * v[1];
    res[2] = m[2] * v[2];
    res[3] = m[3];
    return res;
}

mat4 translate(in mat4 m, in vec3 v) {
    mat4 res;
    res[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return res;
}

void main()
{
    ObjectData object = objectDataBuffer.variables[pushConsts.idx];
    //    outIsOutline=pushConsts.isOutline;
    //    outlineColor=pushConsts.outlineColor;
    //    outColor=pushConsts.inColor;

    outFragTextureCoords=vec3(
    inFragTextureCoords.x * object.textureMultiply.x,
    inFragTextureCoords.y * object.textureMultiply.y,
    object.textureIndex
    );

    outLighting = min(1, 1.2f+sin(float(globalVariableBuffer.variables[0].value)*2*PI/360.0f));

    mat4 translatedOnGrid = translate(vec3(object.position, 1.0f));
    mat4 locallyScaled = scale(vec3(environmentData.tileSize, 1.0f)) * translatedOnGrid;
//    mat4 globallyScaled = locallyScaled * scale(vec3(environmentData.tileSize, 1.0f));
    mat4 mvp = environmentData.projectionMatrix * locallyScaled;

    gl_Position=mvp*vec4(
    inPosition.x,
    inPosition.y,
    inPosition.z,
    1.
    );
}