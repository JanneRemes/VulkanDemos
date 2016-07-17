#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Push Constants block
/*layout(push_constant) uniform PushConstants
{
	mat4 projMatrix;
	float animationTime;
} pushConstants;*/

// Inputs
layout(location = 0) in vec2 position;

// Outputs
layout(location = 0) out vec2 outUV;	// TODO pick better name


void main()
{
	outUV = position;

	//gl_Position = (pushConstants.projMatrix * vec4(position, 1.0));
	gl_Position = vec4(position, 1.0, 1.0);
}
