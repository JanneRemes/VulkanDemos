#ifndef VKDEMOS_SUBMITIMAGEBARRIER_H
#define VKDEMOS_SUBMITIMAGEBARRIER_H

namespace vkdemos {

#include "vulkan/vulkan.h"

/**
 * Appends a CmdPipelineBarrier to the specified command buffer,
 * containing an Image Barrier operation with the specified parameters.
 */
void submitImageBarrier(const VkCommandBuffer theCommandBuffer,
                        const VkImage theImage,
                        const VkAccessFlags srcAccessMask,
                        const VkAccessFlags dstAccessMask,
                        const VkImageLayout oldLayout,
                        const VkImageLayout newLayout,
                        const VkImageSubresourceRange subresourceRange
                        )
{
	VkImageMemoryBarrier imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange = subresourceRange,
		.image = theImage,
	};

	vkCmdPipelineBarrier(theCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // srcStageMask
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // dstStageMask
		0,                                 // dependencyFlags
		0,                                 // memoryBarrierCount
		nullptr,                           // pMemoryBarriers
		0,                                 // bufferMemoryBarrierCount
		nullptr,                           // pBufferMemoryBarriers
		1,                                 // imageMemoryBarrierCount
		&imageMemoryBarrier                // pImageMemoryBarriers
	);
}

}

#endif
