#ifndef DEMO02RENDERSINGLEFRAME_H
#define DEMO02RENDERSINGLEFRAME_H

#include "../00_commons/00_utils.h"
#include "demo02fillrenderingcommandbuffer.h"

#include <vulkan/vulkan.h>
#include <iostream>


/**
 * Renders a single frame for this demo (i.e. we clear the screen).
 * Returns true on success and false on failure.
 *
 * For a more detailed description, refer to demo 01_clearscreen.
 */
bool demo02RenderSingleFrame(const VkDevice theDevice,
                             const VkQueue theQueue,
                             const VkSwapchainKHR theSwapchain,
                             const VkCommandBuffer thePresentCmdBuffer,
                             const std::vector<VkFramebuffer> & theFramebuffersVector,
                             const VkRenderPass theRenderPass,
                             const VkPipeline thePipeline,
                             const VkBuffer theVertexBuffer,
                             const uint32_t vertexInputBinding,
                             const int width,
                             const int height
                             )
{
	VkResult result;
	VkSemaphore imageAcquiredSemaphore, renderingCompletedSemaphore;

	result = vkdemos::utils::createSemaphore(theDevice, imageAcquiredSemaphore);
	assert(result == VK_SUCCESS);

	result = vkdemos::utils::createSemaphore(theDevice, renderingCompletedSemaphore);
	assert(result == VK_SUCCESS);


	/*
	 * Acquire the index of the next available swapchain image.
	 */
	uint32_t imageIndex = UINT32_MAX;
	result = vkAcquireNextImageKHR(theDevice, theSwapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);

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
	bool boolResult = demo02FillRenderingCommandBuffer(thePresentCmdBuffer, theFramebuffersVector[imageIndex], theRenderPass, thePipeline, theVertexBuffer, vertexInputBinding, width, height);
	assert(boolResult);


	/*
	 * Submit the present command buffer to the queue.
	 */
	VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &imageAcquiredSemaphore,
		.pWaitDstStageMask = &pipelineStageFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &thePresentCmdBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderingCompletedSemaphore
	};

	result = vkQueueSubmit(theQueue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(result == VK_SUCCESS);


	/*
	 * Present the rendered image, so that it will be queued for display.
	 */
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderingCompletedSemaphore,
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


	/*
	 * Wait for the queue to complete working.
	 */
	vkQueueWaitIdle(theQueue);

	/*
	 * Cleanup
	 */
	vkDestroySemaphore(theDevice, imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(theDevice, renderingCompletedSemaphore, nullptr);
	return true;
}


#endif
