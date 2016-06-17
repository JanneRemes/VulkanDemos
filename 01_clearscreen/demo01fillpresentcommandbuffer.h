#ifndef DEMO01FILLPRESENTCOMMANDBUFFER_H
#define DEMO01FILLPRESENTCOMMANDBUFFER_H

#include <vulkan/vulkan.h>

#include <cassert>

/**
 * Fill the specified command buffer with the present commands for this demo.
 */
bool demo01FillPresentCommandBuffer(const VkCommandBuffer theCommandBuffer, const VkImage theCurrentSwapchainImage, const float clearColorR, const float clearColorG, const float clearColorB)
{
	VkResult result;

	/*
	 * Begin recording of the command buffer
	 */
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr,
	};

	result = vkBeginCommandBuffer(theCommandBuffer, &commandBufferBeginInfo);
	assert(result == VK_SUCCESS);


	// Pre-fill a VkImageMemoryBarrier structure that we'll use later
	VkImageMemoryBarrier imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
		.image = theCurrentSwapchainImage,
	};

	/*
	 * Transition the swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	 * we don't care what it was before so we set the old layout to VK_IMAGE_LAYOUT_UNDEFINED.
	 */
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(theCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
	);

	/*
	 * Add the CmdClearColorImage that will fill the swapchain image with the specified color.
	 * For this command to work, the destination image must be in either VK_IMAGE_LAYOUT_GENERAL
	 * or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
	 */
	VkClearColorValue clearColorValue;
	VkImageSubresourceRange imageSubresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	clearColorValue.float32[0] = clearColorR;	// FIXME it assumes the image is a floating point one.
	clearColorValue.float32[1] = clearColorG;
	clearColorValue.float32[2] = clearColorB;
	clearColorValue.float32[3] = 1.0f;	// alpha
	vkCmdClearColorImage(theCommandBuffer, theCurrentSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &imageSubresourceRange);


	/*
	 * Transition the swapchain image from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	 * to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	 */
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	vkCmdPipelineBarrier(theCommandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
	);


	/*
	 * End recording of the command buffer
	 */
	result = vkEndCommandBuffer(theCommandBuffer);
	return true;
}

#endif
