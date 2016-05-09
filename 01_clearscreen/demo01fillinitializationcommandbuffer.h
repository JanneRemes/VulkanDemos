#ifndef DEMO01FILLINITIALIZATIONCOMMANDBUFFER_H
#define DEMO01FILLINITIALIZATIONCOMMANDBUFFER_H

#include <vulkan/vulkan.h>

#include <vector>
#include <cassert>

/**
 * Fill the specified command buffer with the initialization commands for this demo.
 *
 * The commands consist in just a bunch of CmdPipelineBarrier that transition the
 * swapchain images from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
 */
bool demo01FillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer, const std::vector<VkImage> & theSwapchainImagesVector)
{
	VkResult result;
	std::vector<VkImageMemoryBarrier> memoryBarriersVector;

	/*
	 * "The pipeline barrier specifies an execution dependency such that all work performed
	 * by the set of pipeline stages included in srcStageMask of the first set of commands
	 * completes before any work performed by the set of pipeline stages
	 * included in dstStageMask of the second set of commands begins." [Section 6.5]
	 *
	 * Pipeline barriers are also the place were we can change layouts of our VkImages;
	 * here we add a Pipeline Barrier for each swapchain image, to transition them
	 * from VK_IMAGE_LAYOUT_UNDEFINED to the correct layout.
	*/
	for(const auto & image : theSwapchainImagesVector)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
			.image = image,
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
