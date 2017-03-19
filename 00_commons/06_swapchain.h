#ifndef VKDEMOS_SWAPCHAIN_H
#define VKDEMOS_SWAPCHAIN_H

namespace vkdemos {

#include "vulkan/vulkan.h"
#include <cassert>
#include <vector>
#include <iostream>

/**
 * Create a VkSwapchain from the VkSurface provided.
 * Parameter "theOldSwapChain" is used if we are recreating a swapchain (for example if we resized the window);
 * if we are creating the swapchain for the first time, this parameter must be VK_NULL_HANDLE.
 */
bool createVkSwapchain(const VkPhysicalDevice thePhysicalDevice,
                       const VkDevice theDevice,
                       const VkSurfaceKHR theSurface,
                       const int windowWidth,
                       const int windowHeight,
                       const int ownedSwapchainImages,
                       VkSwapchainKHR theOldSwapChain,
                       VkSwapchainKHR & outSwapchain,
                       VkFormat & outSurfaceFormat,
                       const VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                       )
{
	VkResult result;

	/*
	 * Get the list of VkSurfaceFormatKHRs that are supported.
	 *
	 * The VkSurfaceFormatKHR struct represents a pair format-colorspace
	 * that specify what colorspace con be used with what surface format.
	 * vkGetPhysicalDeviceSurfaceFormatsKHR will return VkSurfaceFormatKHRs
	 * whose format is compatible with the specified VkSurfaceKHR.
	 */
	uint32_t surfaceFormatsCount;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(thePhysicalDevice, theSurface, &surfaceFormatsCount, nullptr);
	assert(result == VK_SUCCESS && surfaceFormatsCount >= 1);

	std::vector<VkSurfaceFormatKHR> surfaceFormatsVector(surfaceFormatsCount);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(thePhysicalDevice, theSurface, &surfaceFormatsCount, surfaceFormatsVector.data());
	assert(result == VK_SUCCESS);

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format. Otherwise, at least one
	// supported format will be returned.
	VkFormat surfaceFormat;

	if (surfaceFormatsCount == 1 && surfaceFormatsVector[0].format == VK_FORMAT_UNDEFINED)
		surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
	else
		surfaceFormat = surfaceFormatsVector[0].format;

	VkColorSpaceKHR surfaceColorSpace = surfaceFormatsVector[0].colorSpace;


	/*
	 * Get physical device surface capabilities.
	 *
	 * Capabilities are a series of max/min values and usages bits
	 * that describe the limits and functionalities available in the specified surface.
	 * Refer to the Vulkan Specification to learn more.
	 */
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(thePhysicalDevice, theSurface, &surfaceCapabilities);
	assert(result == VK_SUCCESS);


	/*
	 * Get Physical device surface present modes.
	 *
	 * A present mode is how the Device will synchronize itself with the video screen when
	 * it has rendered frames available to display.
	 */
	uint32_t presentModeCount = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(thePhysicalDevice, theSurface, &presentModeCount, nullptr);
	assert(result == VK_SUCCESS && presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModesVector(presentModeCount);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(thePhysicalDevice, theSurface, &presentModeCount, presentModesVector.data());
	assert(result == VK_SUCCESS);

	for(const auto presMode : presentModesVector)
	{
		std::cout << "--- Supported present mode: ";
		switch(presMode) {
			case VK_PRESENT_MODE_IMMEDIATE_KHR:    std::cout << "VK_PRESENT_MODE_IMMEDIATE_KHR";    break;
			case VK_PRESENT_MODE_MAILBOX_KHR:      std::cout << "VK_PRESENT_MODE_MAILBOX_KHR";      break;
			case VK_PRESENT_MODE_FIFO_KHR:         std::cout << "VK_PRESENT_MODE_FIFO_KHR";         break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR: std::cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR"; break;
			default: std::cout << "???"; break;
		}
		std::cout << " (" << presMode << ')' << std::endl;
	}


	// Get the swapchain extent, and check if it's already filled or if we must manually set width and height.
	VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;

	if(swapchainExtent.width == (uint32_t)(-1)) // width and height are either both -1, or both not -1.
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = (uint32_t)windowWidth;
		swapchainExtent.height = (uint32_t)windowHeight;
	}
	else {
		assert(swapchainExtent.width == (uint32_t)windowWidth && swapchainExtent.height == (uint32_t)windowHeight);
	}


	/*
	 * Determine the number of VkImage's to use in the swap chain.
	 */
	uint32_t desiredNumberOfSwapchainImages = surfaceCapabilities.minImageCount + (uint32_t)ownedSwapchainImages;

	if (surfaceCapabilities.maxImageCount > 0) {
		// Limit the number of images, if maxImageCount is defined.
		desiredNumberOfSwapchainImages = std::min(desiredNumberOfSwapchainImages, surfaceCapabilities.maxImageCount);
	}

	/*
	 * VkSurfaceTransformFlagBitsKHR define how the surface is transformed (rotated/mirrored)
	 * before displaying. The supported flags for the current surface are found in
	 * VkSurfaceCapabilitiesKHR::supportedTransforms.
	 */
	VkSurfaceTransformFlagBitsKHR surfaceTransformFlagBits;

	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		surfaceTransformFlagBits = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		surfaceTransformFlagBits = surfaceCapabilities.currentTransform;


	/*
	 * Create the swapchain.
	 * TODO document all the fields
	 */
	const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.surface = theSurface,
		.minImageCount = desiredNumberOfSwapchainImages,
		.imageFormat = surfaceFormat,
		.imageColorSpace = surfaceColorSpace,
		.imageExtent = swapchainExtent,
		.imageUsage = imageUsageFlags,
		.preTransform = surfaceTransformFlagBits,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,  // When compositing to screen, don't use any alpha information present in the image.
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,  // The swapchain's images will be accessed by a single queue at a time.
		.queueFamilyIndexCount = 0,                     // If imageSharingMode is VK_SHARING_MODE_CONCURRENT, provide here
		.pQueueFamilyIndices = nullptr,                 //   and here a vector of queue families that will be allowed to access the swapchain's images.
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,        // Present mode FIFO will wait for vsync if no swapchain images are available.
		.oldSwapchain = theOldSwapChain,                // If we are recreating a swapchain, we pass the old one here.
		.clipped = VK_TRUE,                             // If some part of the surface isn't visible,
	                                                    //   let the Vulkan Implementation discard rendering operations on them.
	};

	VkSwapchainKHR mySwapchain;
	result = vkCreateSwapchainKHR(theDevice, &swapchainCreateInfo, nullptr, &mySwapchain);
	assert(result == VK_SUCCESS);

	std::cout << "+++ VkSwapchainKHR created succesfully!\n";
	outSwapchain = mySwapchain;
	outSurfaceFormat = surfaceFormat;

	// Destroy the old swapchain, if there was one.
	if(theOldSwapChain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(theDevice, theOldSwapChain, nullptr);
		std::cout << "+++     ... and old VkSwapchainKHR destroyed succesfully!\n";
	}
	std::cout << std::endl;

	return true;
}




/**
 * Get the VkImages out of the specified swapchain, and create for each of them an
 * associated VkImageView.
 */
bool getSwapchainImagesAndViews(const VkDevice theDevice,
                                const VkSwapchainKHR theSwapchain,
                                const VkFormat & theSurfaceFormat,
                                std::vector<VkImage> & outSwapchainImagesVector,
                                std::vector<VkImageView> & outSwapchainImageViewsVector
                                )
{
	VkResult result;

	/*
	 * Get swapchain images.
	 */
	uint32_t swapchainImageCount;
	result = vkGetSwapchainImagesKHR(theDevice, theSwapchain, &swapchainImageCount, nullptr);
	assert(result == VK_SUCCESS);

	std::vector<VkImage> swapchainImagesVector(swapchainImageCount);
	result = vkGetSwapchainImagesKHR(theDevice, theSwapchain, &swapchainImageCount, swapchainImagesVector.data());
	assert(result == VK_SUCCESS);

	std::vector<VkImageView> swapchainImageViewsVector(swapchainImageCount);

	/*
	 * Create a view for each image.
	 */
	for(auto i = 0u; i < swapchainImageCount; i++)
	{
		// TODO document every field
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.format = theSurfaceFormat,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			},
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.image = swapchainImagesVector[i],
		};

		result = vkCreateImageView(theDevice, &imageViewCreateInfo, nullptr, &swapchainImageViewsVector[i]);
		assert(result == VK_SUCCESS);
	}

	std::cout << "+++ Created " << swapchainImageCount << " swapchain images and views."<< std::endl;
	outSwapchainImagesVector = std::move(swapchainImagesVector);
	outSwapchainImageViewsVector = std::move(swapchainImageViewsVector);
	return true;
}

}

#endif
