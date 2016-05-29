#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Push Constants block
layout(push_constant) uniform PushConstants
{
	float animationTime;
} pushConstants;

// Combined Image Sampler Binding
layout (binding = 0) uniform sampler2D textureSampler;

// Inputs
layout(location = 0) in vec2 inUV;

// Outputs
layout(location = 0) out vec4 outFragmentColor;


/*
 * hsv2rgb from:
 * http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
 */
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}


void main()
{
	//float hue = fract(pushConstants.animationTime / 7000.0);
	//outFragmentColor = vec4(hsv2rgb(vec3(hue, 1.0, 1.0)), 1.0);

	vec4 sampl = texture(textureSampler, inUV);
	outFragmentColor = vec4(sampl.rgb, 1.0);
}
