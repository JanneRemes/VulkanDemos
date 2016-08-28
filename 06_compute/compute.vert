#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Inputs
layout(location = 0) in vec2 position;

// Outputs
layout(location = 0) out vec2 outUV;


void main()
{
	outUV = (position + 1.0) / 2.0;
	gl_Position = vec4(position, 1.0, 1.0);
}
