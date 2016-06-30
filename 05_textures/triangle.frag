#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Push Constants block
layout(push_constant) uniform PushConstants
{
	mat4 projMatrix;
	float animationTime;
} pushConstants;

// Combined Image Sampler Binding
layout(set = 0, binding = 0) uniform sampler2D textureSampler;	// this sampler is attached to binding point 0 inside descriptor set 0

// Inputs
layout(location = 0) in vec2 inUV;

// Outputs
layout(location = 0) out vec4 outFragmentColor;


void main()
{
	outFragmentColor = texture(textureSampler, inUV);
}
