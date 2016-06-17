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
layout(location = 1) in vec3 inColor;

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

	//float a = gl_PrimitiveID % 2 == 0 ? 1.0 : 0.5;

	vec4 sampl = texture(textureSampler, inUV);

	vec3 corr = inColor.x == 0.0 ? sampl.rgb : sampl.brg;
//	vec3 corr = sampl.rgb;
	/*corr.r = pow(corr.r, 1.0/2.2);
	corr.g = pow(corr.g, 1.0/2.2);
	corr.b = pow(corr.b, 1.0/2.2);*/

	outFragmentColor = vec4(corr, 1.0);
}
