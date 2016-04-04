#ifndef VKDEMOS_CREATEANDALLOCATEBUFFER_H
#define VKDEMOS_CREATEANDALLOCATEBUFFER_H

namespace vkdemos {

#include "00_utils.h"

#include "vulkan/vulkan.h"
#include <cassert>

/**
 * Creates a VkBuffer and allocates memory for it.
 *
 * This function is basically identical to createAndAllocateImage;
 * for an extended explanation of Vulkan's memory binding, refer
 * to createAndAllocateImage's implementation.
 */
bool createAndAllocateBuffer(const VkDevice theDevice,
							 const VkPhysicalDeviceMemoryProperties theMemoryProperties,
							 const VkBufferUsageFlags bufferUsage,
							 const VkMemoryPropertyFlags requiredMemoryProperties,
							 const VkDeviceSize bufferSize,
							 VkBuffer & outBuffer,
							 VkDeviceMemory & outBufferMemory
							 )
{
	VkResult result;
	VkBuffer myBuffer;
	VkDeviceMemory myBufferMemory;

	/*
	 * Create the VkBuffer object.
	 */
	const VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = bufferSize,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // access exclusive to a single queue family at a time
		.queueFamilyIndexCount = 0,                // unused in sharing mode exclusive
		.pQueueFamilyIndices = nullptr,            // unused in sharing mode exclusive
	};

	result = vkCreateBuffer(theDevice, &bufferCreateInfo, nullptr, &myBuffer);
	assert(result == VK_SUCCESS);

	// Get memory requirements for the buffer.
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(theDevice, myBuffer, &memoryRequirements);

	// Find an appropriate memory type with all the requirements for our buffer
	int memoryTypeIndex = vkdemos::utils::findMemoryTypeWithProperties(theMemoryProperties, memoryRequirements.memoryTypeBits, requiredMemoryProperties);
	if(memoryTypeIndex < 0) {
		std::cout << "!!! ERROR: Can't find a memory type to hold the buffer." << std::endl;
		return false;
	}

	/*
	 * Allocate memory for the buffer.
	 */
	const VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = (uint32_t)memoryTypeIndex,
	};

	result = vkAllocateMemory(theDevice, &memoryAllocateInfo, nullptr, &myBufferMemory);
	assert(result == VK_SUCCESS);

	/*
	 * Bind the allocated memory to the buffer.
	 */
	result = vkBindBufferMemory(theDevice, myBuffer, myBufferMemory, 0);
	assert(result == VK_SUCCESS);

	outBuffer = myBuffer;
	outBufferMemory = myBufferMemory;
	return true;
}

}	// vkdemos
#endif
