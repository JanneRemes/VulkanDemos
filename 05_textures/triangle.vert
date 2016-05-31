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
layout(location = 1) out vec3 outColor;


// Constants

// Near and Far plane
const float far = 100.0;
const float near = 1.0;
const int windowWidth = 4;
const int windowHeight = 4;

// Field Of View
const float verticalFov = radians(60);
const float horizontalFov = 2.0 * atan(tan(verticalFov / 2.0) * (windowWidth / windowHeight));

// The Projection Matrix (Remember: the matrix constructor parameters are columns!)
const float r = tan(horizontalFov/2.0);
const float t = tan(verticalFov/2.0);

const mat4 projectionMatrix = mat4(
	vec4(near/r, 0.0, 0.0, 0.0),
	vec4(0.0, near/t, 0.0, 0.0),
	vec4(0.0, 0.0, -(far+near)/(far-near), -1.0),
	vec4(0.0, 0.0, -(2.0*far*near)/(far-near), 0.0)
);


/*
	float radHorFOV = 2.0f * (float)atan(tan(FOVAngleVertical * (M_PI / 180.0f) / 2) * (float(windowWidth) / windowHeight));
	FOVAngleHorizontal = std::min(float(radHorFOV * (180.0f/M_PI)), 180.0f);	// Brutte cose succedono con un hfov > 180°

	// Ricostruiamo la matrice di prospettiva
	float r = (float)tan(rendermgr::getHorizontalFOV()*(M_PI/180.0)/2);
	float t = (float)tan(rendermgr::getVerticalFOV()*(M_PI / 180.0) / 2);

	perspectiveMatrix.setValue(0, 0, 1.0f / r);
	perspectiveMatrix.setValue(1, 1, 1.0f / t);
	perspectiveMatrix.setValue(2, 2, (fzFar + fzNear) / (fzNear - fzFar));
	perspectiveMatrix.setValue(2, 3, (2 * fzFar * fzNear) / (fzNear - fzFar));
	perspectiveMatrix.setValue(3, 2, -1.0f);
*/



void main()
{
	float angle = pushConstants.animationTime / 7000.0;

	mat2 rotation = mat2(
		cos(angle), -sin(angle),
		sin(angle), cos(angle)
	);

	outUV = colorIn.xy;
	outColor = vec3(colorIn.z, 1.0, 1.0);

	gl_Position = projectionMatrix*vec4(position.xyz+vec3(0,0,sin(angle*5)*5), 1.0);
}
