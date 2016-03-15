#ifndef VKDEMOS_COMMANDPOOLANDBUFFER_H
#define VKDEMOS_COMMANDPOOLANDBUFFER_H

namespace vkdemos {

#include "vulkan/vulkan.h"
#include <cassert>

/**
 * Create a VkCommandPool, from which all the VkCommandBuffer will be allocated.
 */
bool createCommandPool(const VkDevice theDevice,
                       const uint32_t theQueueFamilyIndex,
                       const VkCommandPoolCreateFlagBits createFlagBits,
                       VkCommandPool & outCommandPool
                       )
{
	VkResult result;

	const VkCommandPoolCreateInfo commandPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = createFlagBits,
		.queueFamilyIndex = theQueueFamilyIndex,
	};

	VkCommandPool myCommandPool;
	result = vkCreateCommandPool(theDevice, &commandPoolCreateInfo, nullptr, &myCommandPool);
	assert(result == VK_SUCCESS);

	outCommandPool = myCommandPool;
	return true;
}


/**
 * Allocate a VkCommandBuffer from a VkCommandPool.
 */
bool allocateCommandBuffer(const VkDevice theDevice,
                           const VkCommandPool theCommandPool,
                           const VkCommandBufferLevel theCommandBufferLevel,
                           VkCommandBuffer & outCommandBuffer
                           )
{
	VkResult result;

	const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = theCommandPool,
		.level = theCommandBufferLevel,
		.commandBufferCount = 1
	};

	VkCommandBuffer myCommandBuffer;
	result = vkAllocateCommandBuffers(theDevice, &commandBufferAllocateInfo, &myCommandBuffer);
	assert(result == VK_SUCCESS);

	outCommandBuffer = myCommandBuffer;
	return true;
}

}

#endif
