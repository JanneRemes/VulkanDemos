// Demo 01: clearscreen.

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

// Include demo functions from the commons directory
#include "../00_commons/00_utils.h"
#include "../00_commons/01_createVkInstance.h"
#include "../00_commons/02_debugReportCallback.h"
#include "../00_commons/03_createVkSurface.h"
#include "../00_commons/04_chooseVkPhysicalDevice.h"
#include "../00_commons/05_createVkDeviceAndVkQueue.h"
#include "../00_commons/06_swapchain.h"
#include "../00_commons/07_commandPoolAndBuffer.h"

// Includes for this file
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cassert>
#include <climits>


/*
 * Prototypes for functions defined in this file.
 */
bool fillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer, const std::vector<VkImage> & theSwapchainImagesVector);
bool fillPresentCommandBuffer(const VkCommandBuffer theCommandBuffer, const VkImage theCurrentSwapchainImage, const float clearColorR, const float clearColorG, const float clearColorB);
bool renderSingleFrame(const VkDevice theDevice, const VkQueue theQueue, const VkSwapchainKHR theSwapchain, const VkCommandBuffer thePresentCmdBuffer, const std::vector<VkImage> & theSwapchainImagesVector, const float clearColorR, const float clearColorG, const float clearColorB);


/**
 * Good ol' main function.
 */
int main(int argc, char* argv[])
{
	static int windowWidth = 800;
	static int windowHeight = 600;
	static const char * applicationName = "SdlVulkanDemo_01_clearscreen";
	static const char * engineName = applicationName;

	bool boolResult;
	VkResult result;

	/*
	 * SDL2 Initialization
	 */
	SDL_Window *mySdlWindow;
	SDL_SysWMinfo mySdlSysWmInfo;

	boolResult = vkdemos::utils::sdl2Initialization(applicationName, windowWidth, windowHeight, mySdlWindow, mySdlSysWmInfo);
	assert(boolResult);

	/*
	 * Do all the Vulkan goodies here.
	 */

	// Vector of the layer names we want to enable on the Instance
	std::vector<const char *> layersNamesToEnable;
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_standard_validation");

	// Vector of the extension names we want to enable on the Instance
	std::vector<const char *> extensionsNamesToEnable;
	extensionsNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME); // TODO: add support for other windowing systems

	// Create a VkInstance
	VkInstance myInstance;
	boolResult = vkdemos::createVkInstance(layersNamesToEnable, extensionsNamesToEnable, applicationName, engineName, myInstance);
	assert(boolResult);

	// Initialize the debug callback
	VkDebugReportCallbackEXT myDebugReportCallback;
	vkdemos::createDebugReportCallback(myInstance,
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		vkdemos::debugCallback,
		myDebugReportCallback
	);

	// Choose a physical device from the instance
	// For simplicity, just pick the first physical device listed (0).
	VkPhysicalDevice myPhysicalDevice;
	boolResult = vkdemos::chooseVkPhysicalDevice(myInstance, 0, myPhysicalDevice);
	assert(boolResult);

	// Create a VkSurfaceKHR
	VkSurfaceKHR mySurface;
	boolResult = vkdemos::createVkSurface(myInstance, mySdlSysWmInfo, mySurface);
	assert(boolResult);

	// Create a VkDevice and its VkQueue
	VkDevice myDevice;
	VkQueue myQueue;
	uint32_t myQueueFamilyIndex;
	boolResult = vkdemos::createVkDeviceAndVkQueue(myPhysicalDevice, mySurface, layersNamesToEnable, myDevice, myQueue, myQueueFamilyIndex);
	assert(boolResult);

	// Create a VkSwapchainKHR
	VkSwapchainKHR mySwapchain;
	VkFormat mySurfaceFormat;
	boolResult = vkdemos::createVkSwapchain(myPhysicalDevice, myDevice, mySurface, windowWidth, windowHeight, 1, VK_NULL_HANDLE, mySwapchain, mySurfaceFormat);
	assert(boolResult);

	// Create the swapchain images and related views.
	// (the views are not actually used in this demo, but since they'll be needed in the future
	// to create the framebuffers, we'll create them anyway to show how to do it).
	std::vector<VkImage> mySwapchainImagesVector;
	std::vector<VkImageView> mySwapchainImageViewsVector;
	boolResult = vkdemos::getSwapchainImagesAndViews(myDevice, mySwapchain, mySurfaceFormat, mySwapchainImagesVector, mySwapchainImageViewsVector);
	assert(boolResult);

	// Create a command pool, so that we can later allocate the command buffers.
	VkCommandPool myCommandPool;
	boolResult = vkdemos::createCommandPool(myDevice, myQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, myCommandPool);
	assert(boolResult);


	// Allocate a command buffer that will hold our initialization commands.
	VkCommandBuffer myCmdBufferInitialization;
	boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, myCmdBufferInitialization);
	assert(boolResult);

	// Allocate a command buffer that will hold our clear screen and present commands.
	VkCommandBuffer myCmdBufferPresent;
	boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, myCmdBufferPresent);
	assert(boolResult);


	/*
	 * We completed the creation and allocation of all the resources we need!
	 * Now it's time to build and submit the first command buffer that will contain
	 * all the initialization commands, such as transitioning the images from
	 * VK_IMAGE_LAYOUT_UNDEFINED to something sensible.
	 */

	// We fill the initialization command buffer with... the initialization commands.
	boolResult = fillInitializationCommandBuffer(myCmdBufferInitialization, mySwapchainImagesVector);
	assert(boolResult);

	// We now submit the command buffer to the queue we created before, and we wait
	// for its completition.
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &myCmdBufferInitialization,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	result = vkQueueSubmit(myQueue, 1, &submitInfo, VK_NULL_HANDLE);
	assert(result == VK_SUCCESS);

	// Wait for the queue to complete its work.
	// Note: in a "real world" application you wouldn't use this function, but you would
	// use a semaphore (a pointer to which goes in the above VkSubmitInfo struct)
	// that will get signalled once the queue completes all the previous commands.
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);

	std::cout << "\n---- Rendering Start ----" << std::endl;

	/*
	 * Now that initialization is complete, we can start our program's event loop!
	 *
	 * We'll process the SDL's events, and then send the drawing/present commands.
	 * (in this demo we just clear the screen and present).
	 *
	 * Just for fun, we also collect and print some statistics about the average time
	 * it takes to draw a single frame.
	 * We also clear the screen to different colors for various frames, so that we
	 * see something changing on the screen.
	 */
	SDL_Event sdlEvent;
	bool quit = false;

	// Just some variables for frame statistics and different colors.
	long frameNumber = 0;
	long frameMaxTime = LONG_MIN;
	long frameMinTime = LONG_MAX;
	long frameAvgTimeSum = 0;
	long frameAvgTimeSumSquare = 0;
	constexpr long FRAMES_PER_STAT = 120;	// How many frames to wait before printing frame time statistics.

	constexpr int MAX_COLORS = 4;
	constexpr int FRAMES_PER_COLOR = 120;	// How many frames to show each color.
	static float screenColors[MAX_COLORS][3] = {
	    {1.0f, 0.2f, 0.2f},
	    {0.0f, 0.9f, 0.2f},
	    {0.0f, 0.2f, 1.0f},
	    {1.0f, 0.9f, 0.2f},
	};

	// The main event/render loop.
	while(!quit)
	{
		// Process events for this frame
		while(SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT) {
				quit = true;
			}
			if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
				quit = true;
			}
		}

		// Rendering code
		if(!quit)
		{
			// Render various colors
			float colR = screenColors[(frameNumber/FRAMES_PER_COLOR) % MAX_COLORS][0];
			float colG = screenColors[(frameNumber/FRAMES_PER_COLOR) % MAX_COLORS][1];
			float colB = screenColors[(frameNumber/FRAMES_PER_COLOR) % MAX_COLORS][2];

			// Render a single frame
			auto renderStartTime = std::chrono::high_resolution_clock::now();
			quit = !renderSingleFrame(myDevice, myQueue, mySwapchain, myCmdBufferPresent, mySwapchainImagesVector, colR, colG, colB);
			auto renderStopTime = std::chrono::high_resolution_clock::now();

			// Compute frame time statistics
			auto elapsedTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(renderStopTime - renderStartTime).count();

			frameMaxTime = std::max(frameMaxTime, elapsedTimeUs);
			frameMinTime = std::min(frameMinTime, elapsedTimeUs);
			frameAvgTimeSum += elapsedTimeUs;
			frameAvgTimeSumSquare += elapsedTimeUs*elapsedTimeUs;

			// Print statistics if necessary
			if(frameNumber % FRAMES_PER_STAT == 0)
			{
				auto average = frameAvgTimeSum/FRAMES_PER_STAT;
				auto stddev = std::sqrt(frameAvgTimeSumSquare/FRAMES_PER_STAT - average*average);
				std::cout << "Frame time: average " << std::setw(6) << average
				          << " us, maximum " << std::setw(6) << frameMaxTime
				          << " us, minimum " << std::setw(6) << frameMinTime
				          << " us, stddev " << (long)stddev
				          << " (" << std::fixed << std::setprecision(2) << (stddev/average * 100.0f) << "%)"
				          << std::endl;

				frameMaxTime = LONG_MIN;
				frameMinTime = LONG_MAX;
				frameAvgTimeSum = 0;
				frameAvgTimeSumSquare = 0;
			}

			frameNumber++;
		}
	}


	/*
	 * Deinitialization
	 */
	// We wait for pending operations to complete before starting to destroy stuff.
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);

	// You don't need to call vkFreeCommandBuffers for all command buffers; all command buffers
	// allocated from a command pool are released when the command pool is destroyed.
	vkDestroyCommandPool(myDevice, myCommandPool, nullptr);

	// Destroy the swapchain image's views.
	for(auto imgView : mySwapchainImageViewsVector)
		vkDestroyImageView(myDevice, imgView, nullptr);

	// NOTE! DON'T destroy the swapchain images, because they are already destroyed
	// during the destruction of the swapchain.
	vkDestroySwapchainKHR(myDevice, mySwapchain, nullptr);

	// There's no function for destroying a queue; all queues of a particular
	// device are destroyed when the device is destroyed.
	vkDestroyDevice(myDevice, nullptr);
	vkDestroySurfaceKHR(myInstance, mySurface, nullptr);
	vkdemos::destroyDebugReportCallback(myInstance, myDebugReportCallback);
	vkDestroyInstance(myInstance, nullptr);

	// Quit SDL
	SDL_DestroyWindow(mySdlWindow);
	SDL_Quit();

	return 0;
}



/**
 * Fill the specified command buffer with the initialization commands for this demo.
 *
 * The commands consist in just a bunch of CmdPipelineBarrier that transition the
 * swapchain images from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
 */
bool fillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer, const std::vector<VkImage> & theSwapchainImagesVector)
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



/**
 * Fill the specified command buffer with the present commands for this demo.
 */
bool fillPresentCommandBuffer(const VkCommandBuffer theCommandBuffer, const VkImage theCurrentSwapchainImage, const float clearColorR, const float clearColorG, const float clearColorB)
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
	 * Transition the swapchain image from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	 * to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	 */
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
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



/**
 * Renders a single frame for this demo (i.e. we clear the screen).
 * Returns true on success and false on failure.
 */
bool renderSingleFrame(const VkDevice theDevice,
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
	bool boolResult = fillPresentCommandBuffer(thePresentCmdBuffer, theSwapchainImagesVector[imageIndex], clearColorR, clearColorG, clearColorB);
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
