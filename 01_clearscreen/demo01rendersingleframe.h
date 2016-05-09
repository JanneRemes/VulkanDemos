#ifndef DEMO01RENDERSINGLEFRAME_H
#define DEMO01RENDERSINGLEFRAME_H

#include <vulkan/vulkan.h>

#include "../00_commons/00_utils.h"
#include "demo01fillpresentcommandbuffer.h"

#include <vector>
#include <cassert>

/**
 * Renders a single frame for this demo (i.e. we clear the screen).
 * Returns true on success and false on failure.
 */
bool demo01RenderSingleFrame(const VkDevice theDevice,
                       const VkQueue theQueue,
                       const VkSwapchainKHR theSwapchain,
                       const VkCommandBuffer thePresentCmdBuffer,
                       const std::vector<VkImage> & theSwapchainImagesVector,
                       const float clearColorR, const float clearColorG, const float clearColorB)
{
	VkResult result;
	VkSemaphore imageAcquiredSemaphore, renderingCompletedSemaphore;

	// Create a semaphore that will be signalled when a swapchain image is ready to use,
	// and that will be waited upon by the queue before starting all the rendering/present commands.
	//
	// Note: in a "real" application, you would create the semaphore only once at program initialization,
	// and not every frame (for performance reasons).
	result = vkdemos::utils::createSemaphore(theDevice, imageAcquiredSemaphore);
	assert(result == VK_SUCCESS);

	// Create another semaphore that will be signalled when the queue has terminated the rendering commands,
	// and that will be waited upon by the actual present operation.
	result = vkdemos::utils::createSemaphore(theDevice, renderingCompletedSemaphore);
	assert(result == VK_SUCCESS);

	/*
	 * Acquire the index of the next available swapchain image.
	 */
	uint32_t imageIndex = UINT32_MAX;
	result = vkAcquireNextImageKHR(theDevice, theSwapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);

	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		// The swapchain is out of date (e.g. the window was resized) and must be recreated.
		// In this demo we just "gracefully crash".
		std::cout << "!!! ERROR: Demo doesn't yet support out-of-date swapchains." << std::endl;
		// TODO tear down and recreate the swapchain and all its images from scratch.
		return false;
	}
	else if(result == VK_SUBOPTIMAL_KHR) {
		// The swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
		std::cout << "~~~ Swapchain is suboptimal." << std::endl;
	}
	else
		assert(result == VK_SUCCESS);


	/*
	 * Fill the present command buffer with... the present commands.
	 */
	bool boolResult = demo01FillPresentCommandBuffer(thePresentCmdBuffer, theSwapchainImagesVector[imageIndex], clearColorR, clearColorG, clearColorB);
	assert(boolResult);


	/*
	 * Submit the present command buffer to the queue (this is the fun part!)
	 *
	 * In the VkSubmitInfo we specify imageAcquiredSemaphore as a wait semaphore;
	 * this way, the submitted command buffers won't start executing before the
	 * swapchain image is ready.
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
	 * Present the rendered image,
	 * so that it will be queued for display.
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
	 * Wait for the operations on the queue to end before cleaning up resources.
	 */
	vkQueueWaitIdle(theQueue);

	// Cleanup
	vkDestroySemaphore(theDevice, imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(theDevice, renderingCompletedSemaphore, nullptr);
	return true;
}

#endif
