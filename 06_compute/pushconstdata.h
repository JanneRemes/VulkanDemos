#ifndef PUSHCONSTDATA_H
#define PUSHCONSTDATA_H

#include "../00_commons/glm/glm/mat4x4.hpp"

/*
 * Data for push constants
 */
struct PushConstData
{
	glm::mat4 projMatrix;
	float animationTime = 0;
};

#endif // PUSHCONSTDATA_H
