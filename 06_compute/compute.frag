#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

const int BORDER_SIZE_PX = 2;
const vec3 BORDER_COLOR = vec3(0.1, 0.1, 0.1);


// Push Constants block
layout(push_constant) uniform PushConstants
{
	ivec2 windowSize;
	ivec2 arenaSize;
} pushConstants;

layout (set = 0, binding = 0, r8ui) uniform readonly uimage2D arenaState;

// Inputs
layout(location = 0) in vec2 inUV;

// Outputs
layout(location = 0) out vec4 outFragmentColor;


void main()
{
	ivec2 cellPos = ivec2(inUV * pushConstants.arenaSize);
	ivec2 pixelInCell = pushConstants.windowSize % (pushConstants.windowSize / pushConstants.arenaSize);


	uint cellValue = imageLoad(arenaState, cellPos).x;

	if( (cellPos.x % 2) == (cellPos.y % 2) )
		outFragmentColor = vec4(0.7, 1.0, 0.7, 1.0) * (cellValue==0 ? 1.0 : 0.10);
	else
		outFragmentColor = vec4(1.0, 0.7, 0.7, 1.0) * (cellValue==0 ? 1.0 : 0.10);
}
