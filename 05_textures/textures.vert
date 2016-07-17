#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Push Constants block
layout(push_constant) uniform PushConstants
{
	mat4 projMatrix;
	float animationTime;
} pushConstants;

// Inputs
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 inUV;

// Outputs
layout(location = 0) out vec2 outUV;


void main()
{
	outUV = inUV;

	gl_Position = (pushConstants.projMatrix * vec4(position, 1.0));
}
