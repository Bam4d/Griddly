#version 450

layout(location=0)in vec3 inPosition;
layout(location=1)in vec2 inFragTextureCoords;

layout(location=0)out vec3 outColor;
layout(location=1)out vec3 outFragTextureCoords;

out gl_PerVertex{
	vec4 gl_Position;
};

layout(push_constant)uniform PushConsts{
	mat4 mvp;
	vec3 inColor;
	int textureIndex;
	float textureMultiplyY;
	float textureMultiplyX;
}pushConsts;

void main()
{
	outColor=pushConsts.inColor;
	outFragTextureCoords=vec3(
		inFragTextureCoords.x*pushConsts.textureMultiplyX,
		inFragTextureCoords.y*pushConsts.textureMultiplyY,
		pushConsts.textureIndex
	);
	gl_Position=pushConsts.mvp*vec4(inPosition.xyz,1.);
}