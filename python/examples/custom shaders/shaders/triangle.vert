#version 460

layout(location=0)in vec3 inPosition;

layout(location=0)out vec4 outColor;

out gl_PerVertex{
	vec4 gl_Position;
};

layout(push_constant)uniform PushConsts{
	mat4 mvp;
	vec4 inColor;
}pushConsts;

void main()
{
	outColor=pushConsts.inColor;
	
	gl_Position=pushConsts.mvp*vec4(inPosition.xyz,1.);
}