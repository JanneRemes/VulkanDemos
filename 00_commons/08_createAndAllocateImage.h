#ifndef VKDEMOS_CREATEANDALLOCATEIMAGE_H
#define VKDEMOS_CREATEANDALLOCATEIMAGE_H

namespace vkdemos {

#include "00_utils.h"

#include "vulkan/vulkan.h"
#include <cassert>

/**
 * Creates a VkImage and allocates memory for it.
 * The image is created with layout VK_IMAGE_LAYOUT_UNDEFINED,
 * and must be transitioned to the appropriate layout.
 *
 * If outImageViewPtr is not nullptr, a VkImageView for the new image is created.
 */
bool createAndAllocateImage(const VkDevice theDevice,
							const VkPhysicalDeviceMemoryProperties theMemoryProperties,
							const VkBufferUsageFlags imageUsage,
							const VkMemoryPropertyFlags requiredMemoryProperties,
							const VkFormat theImageFormat,
							const int width,
							const int height,
							VkImage & outImage,
							VkDeviceMemory & outImageMemory,
							VkImageView * outImageViewPtr = nullptr,
							VkImageAspectFlags viewSubresourceAspectMask = 0
							)
{
	VkResult result;
	VkImage myImage;
	VkImageView myImageView;
	VkDeviceMemory myImageMemory;

	/*
	 * Create the VkImage.
	 *
	 * When we create the VkImage object with vkCreateImage, we only create a
	 * CPU-side object, that contains all the metadata of the image, but
	 * no GPU memory is allocated for storing its contents.
	 */
	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = theImageFormat,
		.extent = {(uint32_t)width, (uint32_t)height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = imageUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // access exclusive to a single queue family at a time
		.queueFamilyIndexCount = 0,                // unused in sharing mode exclusive
		.pQueueFamilyIndices = nullptr,            // unused in sharing mode exclusive
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	result = vkCreateImage(theDevice, &imageCreateInfo, nullptr, &myImage);
	assert(result == VK_SUCCESS);


	/*
	 * Allocate memory for the image.
	 *
	 * To allocate the necessary GPU memory to store the contents of our image,
	 * we must find an appropriate memory heap that supports all the requirements
	 * (alignment, GPU/CPU visiblity etc) that our image needs.
	 *
	 * With vkGetImageMemoryRequirements, we query the VkImage object we just created,
	 * so that we then know what type of memory heap to search for in the VkDevice.
	 */

	// Get the memory requirements for our image.
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(theDevice, myImage, &memoryRequirements);

	// Find an appropriate memory type with all the requirements for our image.
	int memoryTypeIndex = vkdemos::utils::findMemoryTypeWithProperties(theMemoryProperties, memoryRequirements.memoryTypeBits, requiredMemoryProperties);
	if(memoryTypeIndex < 0) {
		std::cout << "!!! ERROR: Can't find a memory type to hold the image." << std::endl;
		return false;
	}

	// Allocate memory for the image.
	const VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = (uint32_t)memoryTypeIndex,
	};

	result = vkAllocateMemory(theDevice, &memoryAllocateInfo, nullptr, &myImageMemory);
	assert(result == VK_SUCCESS);


	/*
	 * Bind the allocated memory to the image.
	 *
	 * After we created a VkImage, queried its memory requirements, and allocated
	 * some appropriate VkDeviceMemory, we can tell Vulkan to use that VkDeviceMemory
	 * as a storage area for our VkImage object.
	 * This connection is established with vkBindImageMemory.
	 *
	 * Note that you could allocate a VkDeviceMemory much larger than needed for a single
	 * VkImage: this way you can bind many VkImages (with the same memory requirements)
	 * on different offsets inside the VkDeviceMemory.
	 * This is done mainly to ammortize the cost of memory allocation (there is a potentially large
	 * space and time overhead in each memory allocation), or it can be used for advanced techniques
	 * such as memory aliasing.
	 * In these demoes we just allocate a new VkDeviceMemory for each single VkImage, for simplicity.
	 */
	result = vkBindImageMemory(theDevice, myImage, myImageMemory, 0);
	assert(result == VK_SUCCESS);

	outImage = myImage;
	outImageMemory = myImageMemory;

	/*
	 * Create a View for the image.
	 */
	if(outImageViewPtr != nullptr)
	{
		const VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = myImage,
			.format = theImageFormat,
			.subresourceRange = {
				.aspectMask = viewSubresourceAspectMask,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
		};

		result = vkCreateImageView(theDevice, &imageViewCreateInfo, nullptr, &myImageView);
		assert(result == VK_SUCCESS);

		*outImageViewPtr = myImageView;
	}

	return true;
}

}	// vkdemos

#endif
