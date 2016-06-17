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

#include "demo01rendersingleframe.h"

// Includes for this file
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cassert>
#include <climits>



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
			quit = !demo01RenderSingleFrame(myDevice, myQueue, mySwapchain, myCmdBufferPresent, mySwapchainImagesVector, colR, colG, colB);
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

