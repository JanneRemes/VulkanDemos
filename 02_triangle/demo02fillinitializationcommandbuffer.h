#ifndef DEMO02FILLINITIALIZATIONCOMMANDBUFFER_H
#define DEMO02FILLINITIALIZATIONCOMMANDBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <cassert>

/**
 * Fill the specified command buffer with the initialization commands for this demo.
 *
 */
bool demo02FillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer,
                                           const VkImage theDepthImage
                                           )
{
	VkResult result;
	std::vector<VkImageMemoryBarrier> memoryBarriersVector;

	/*
	 * We prepare the depth buffer's image, transitioning it
	 * to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
	 */
	{
		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
			.image = theDepthImage,
		};

		memoryBarriersVector.push_back(imageMemoryBarrier);
	}


	/*
	 * Now we submit all the Pipeline Barriers to a command buffer
	 * through the vkCmdPipelineBarrier command.
	 */
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = (VkCommandBufferUsageFlags)0,
		.pInheritanceInfo = nullptr,
	};

	// Begin command buffer recording.
	result = vkBeginCommandBuffer(theCommandBuffer, &commandBufferBeginInfo);
	assert(result == VK_SUCCESS);

	// Submit the Pipeline Barriers
	vkCmdPipelineBarrier(theCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,     // srcStageMask
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,     // dstStageMask
		0,                                     // dependencyFlags
		0,                                     // memoryBarrierCount
		nullptr,                               // pMemoryBarriers
		0,                                     // bufferMemoryBarrierCount
		nullptr,                               // pBufferMemoryBarriers
		(uint32_t)memoryBarriersVector.size(), // imageMemoryBarrierCount
		memoryBarriersVector.data()            // pImageMemoryBarriers
	);

	// End command buffer recording.
	result = vkEndCommandBuffer(theCommandBuffer);
	return true;
}

#endif
