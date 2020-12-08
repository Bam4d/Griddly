#version 450

layout(location=0)in vec3 inPosition;
layout(location=1)in vec2 inFragTextureCoords;

layout(location=0)out vec4 outColor;
layout(location=1)out vec3 outFragTextureCoords;
layout(location=2)out int outIsOutline;
layout(location=3)out vec4 outlineColor;

out gl_PerVertex{
	vec4 gl_Position;
};

layout(push_constant)uniform PushConsts{
	mat4 mvp;
	vec4 inColor;
	int textureIndex;
	float textureMultiplyY;
	float textureMultiplyX;
	int isOutline;
	vec4 outlineColor;
}pushConsts;

void main()
{
	outIsOutline=pushConsts.isOutline;
	outlineColor=pushConsts.outlineColor;
	outColor=pushConsts.inColor;
	
	outFragTextureCoords=vec3(
		inFragTextureCoords.x*pushConsts.textureMultiplyX,
		inFragTextureCoords.y*pushConsts.textureMultiplyY,
		pushConsts.textureIndex
	);
	
	gl_Position=pushConsts.mvp*vec4(inPosition.xyz,1.);
}