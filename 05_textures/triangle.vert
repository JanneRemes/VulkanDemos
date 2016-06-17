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
layout(location = 1) in vec3 colorIn;

// Outputs
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outColor;


void main()
{
	outUV = colorIn.xy;
	outColor = vec3(colorIn.z, 1.0, 1.0);

	gl_Position = (pushConstants.projMatrix * vec4(position.xyz, 1.0));
}
