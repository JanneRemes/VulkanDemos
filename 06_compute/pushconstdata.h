#ifndef PUSHCONSTDATA_H
#define PUSHCONSTDATA_H

#include "../00_commons/glm/glm/vec2.hpp"

/*
 * Data for push constants
 */
struct PushConstData
{
	glm::ivec2 windowSize;
	glm::ivec2 arenaSize;
};

#endif // PUSHCONSTDATA_H
