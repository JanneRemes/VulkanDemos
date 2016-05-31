#ifndef DEMO05RENDERSINGLEFRAME_H
#define DEMO05RENDERSINGLEFRAME_H

#include "demo05fillrenderingcommandbuffer.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <cassert>


struct PerFrameData
{
	VkCommandBuffer presentCmdBuffer;
	VkSemaphore imageAcquiredSemaphore;
	VkSemaphore renderingCompletedSemaphore;
	VkFence presentFence;
	bool fenceInitialized;
};


/**
 * Renders a single frame.
 * Returns true on success and false on failure.
 */
bool demo05RenderSingleFrame(const VkDevice theDevice,
                             const VkQueue theQueue,
                             const VkSwapchainKHR theSwapchain,
                             const std::vector<VkFramebuffer> & theFramebuffersVector,
                             const VkRenderPass theRenderPass,
                             const VkPipeline thePipeline,
                             const VkPipelineLayout thePipelineLayout,
                             const VkBuffer theVertexBuffer,
                             const uint32_t vertexInputBinding,
                             const uint32_t numberOfVertices,
                             const VkDescriptorSet theDescriptorSet,
                             PerFrameData & thePerFrameData,
                             const int width,
                             const int height,
                             const float animationTime
                             )
{
	VkResult result;

	/*
	 * Wait on the previous frame's fence so that we don't render frames too fast.
	 */
	if(thePerFrameData.fenceInitialized) {
		vkWaitForFences(theDevice, 1, &thePerFrameData.presentFence, VK_TRUE, UINT64_MAX);
		vkResetFences(theDevice, 1, &thePerFrameData.presentFence);
	}


	/*
	 * Acquire the index of the next available swapchain image.
	 */
	uint32_t imageIndex = UINT32_MAX;
	result = vkAcquireNextImageKHR(theDevice, theSwapchain, UINT64_MAX, thePerFrameData.imageAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);

	thePerFrameData.fenceInitialized = true;

	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		std::cout << "!!! ERROR: Demo doesn't yet support out-of-date swapchains." << std::endl;
		return false;
	}
	else if(result == VK_SUBOPTIMAL_KHR) {
		std::cout << "~~~ Swapchain is suboptimal." << std::endl;
	}
	else
		assert(result == VK_SUCCESS);


	/*
	 * Fill the present command buffer with... the present commands.
	 */
	bool boolResult = demo05FillRenderingCommandBuffer(thePerFrameData.presentCmdBuffer, theFramebuffersVector[imageIndex], theRenderPass, thePipeline, thePipelineLayout, theVertexBuffer, vertexInputBinding, numberOfVertices, theDescriptorSet, width, height, animationTime);
	assert(boolResult);


	/*
	 * Submit the present command buffer to the queue.
	 */
	VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &thePerFrameData.imageAcquiredSemaphore,
		.pWaitDstStageMask = &pipelineStageFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &thePerFrameData.presentCmdBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &thePerFrameData.renderingCompletedSemaphore
	};

	result = vkQueueSubmit(theQueue, 1, &submitInfo, thePerFrameData.presentFence);
	assert(result == VK_SUCCESS);


	/*
	 * Present the rendered image, so that it will be queued for display.
	 */
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &thePerFrameData.renderingCompletedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &theSwapchain,
		.pImageIndices = &imageIndex,
		.pResults = nullptr,
	};

	result = vkQueuePresentKHR(theQueue, &presentInfo);

	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		std::cout << "!!! ERROR: Demo doesn't yet support out-of-date swapchains." << std::endl;
		return false;
	}
	else if(result != VK_SUBOPTIMAL_KHR)
		assert(result == VK_SUCCESS);


	return true;
}


#endif
