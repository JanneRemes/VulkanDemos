#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Push Constants block
layout(push_constant) uniform PushConstants
{
	float animationTime;
} pushConstants;

// Inputs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 colorIn;

// Outputs
layout(location = 0) out vec2 outUV;


void main()
{
	float angle = pushConstants.animationTime / 7000.0;
angle = 0;
	mat2 rotation = mat2(
		cos(angle), -sin(angle),
		sin(angle), cos(angle)
	);


	outUV = colorIn.xy;

	gl_Position = vec4(rotation*position.xy, 1.0, 1.0);
}
