// Demo 04: Push Constants.

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
#include "../00_commons/08_createAndAllocateImage.h"
#include "../00_commons/09_createAndAllocateBuffer.h"

#include "demo04rendersingleframe.h"

// CreatePipeline and CreateRenderPass are the same as Demo 02
#include "../02_triangle/demo02createpipeline.h"
#include "../02_triangle/demo02createrenderpass.h"
#include "../02_triangle/demo02fillinitializationcommandbuffer.h"

// Includes for this file
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cassert>
#include <climits>
#include <cstddef>
#include <cmath>


/*
 * Constants
 */
static const int FRAME_LAG = 2;

static const std::string VERTEX_SHADER_FILENAME   = "vertex.spirv";
static const std::string FRAGMENT_SHADER_FILENAME = "fragment.spirv";

static constexpr int VERTEX_INPUT_BINDING = 0;

// Vertex data to draw.
static constexpr int NUM_DEMO_VERTICES = 3;
static const TriangleDemoVertex vertices[NUM_DEMO_VERTICES] =
{
	//      position             color
	{  0.433f, 0.25f,  0.0f,  0.1f, 0.8f, 0.1f },
	{ -0.433f, 0.25f,  0.0f,  0.8f, 0.1f, 0.1f },
	{  0.0f  , -0.5f,  0.0f,  0.1f, 0.1f, 0.8f },
};


/**
 * Good ol' main function.
 */
int main(int argc, char* argv[])
{
	static int windowWidth = 600;
	static int windowHeight = 600;
	static const char * applicationName = "SdlVulkanDemo_04_push_constants";
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
	 * Vulkan initialization.
	 */
	std::vector<const char *> layersNamesToEnable;
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_standard_validation");

	std::vector<const char *> extensionsNamesToEnable;
	extensionsNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME); // TODO: add support for other windowing systems

	VkInstance myInstance;
	boolResult = vkdemos::createVkInstance(layersNamesToEnable, extensionsNamesToEnable, applicationName, engineName, myInstance);
	assert(boolResult);

	VkDebugReportCallbackEXT myDebugReportCallback;
	vkdemos::createDebugReportCallback(myInstance,
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		vkdemos::debugCallback,
		myDebugReportCallback
	);

	VkPhysicalDevice myPhysicalDevice;
	boolResult = vkdemos::chooseVkPhysicalDevice(myInstance, 0, myPhysicalDevice);
	assert(boolResult);

	VkSurfaceKHR mySurface;
	boolResult = vkdemos::createVkSurface(myInstance, mySdlSysWmInfo, mySurface);
	assert(boolResult);

	VkDevice myDevice;
	VkQueue myQueue;
	uint32_t myQueueFamilyIndex;
	boolResult = vkdemos::createVkDeviceAndVkQueue(myPhysicalDevice, mySurface, layersNamesToEnable, myDevice, myQueue, myQueueFamilyIndex);
	assert(boolResult);

	VkSwapchainKHR mySwapchain;
	VkFormat mySurfaceFormat;
	boolResult = vkdemos::createVkSwapchain(myPhysicalDevice, myDevice, mySurface, windowWidth, windowHeight, FRAME_LAG, VK_NULL_HANDLE, mySwapchain, mySurfaceFormat);
	assert(boolResult);

	std::vector<VkImage> mySwapchainImagesVector;
	std::vector<VkImageView> mySwapchainImageViewsVector;
	boolResult = vkdemos::getSwapchainImagesAndViews(myDevice, mySwapchain, mySurfaceFormat, mySwapchainImagesVector, mySwapchainImageViewsVector);
	assert(boolResult);

	VkCommandPool myCommandPool;
	boolResult = vkdemos::createCommandPool(myDevice, myQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, myCommandPool);
	assert(boolResult);

	VkCommandBuffer myCmdBufferInitialization;
	boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, myCmdBufferInitialization);
	assert(boolResult);

	VkPhysicalDeviceMemoryProperties myMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(myPhysicalDevice, &myMemoryProperties);

	// Create the Depth Buffer's Image and View.
	const VkFormat myDepthBufferFormat = VK_FORMAT_D16_UNORM;

	VkImage myDepthImage;
	VkImageView myDepthImageView;
	VkDeviceMemory myDepthMemory;
	boolResult = vkdemos::createAndAllocateImage(myDevice,
	                                    myMemoryProperties,
	                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	                                    0,
	                                    myDepthBufferFormat,
	                                    windowWidth,
	                                    windowHeight,
	                                    myDepthImage,
	                                    myDepthMemory,
	                                    &myDepthImageView,
	                                    VK_IMAGE_ASPECT_DEPTH_BIT
	                                    );
	assert(boolResult);

	// Create the renderpass.
	VkRenderPass myRenderPass;
	boolResult = demo02CreateRenderPass(myDevice, mySurfaceFormat, myDepthBufferFormat, myRenderPass);
	assert(boolResult);

	// Create the Framebuffers, based on the number of swapchain images.
	std::vector<VkFramebuffer> myFramebuffersVector;
	myFramebuffersVector.reserve(mySwapchainImageViewsVector.size());

	for(const auto view : mySwapchainImageViewsVector) {
		VkFramebuffer fb;
		boolResult = vkdemos::utils::createFramebuffer(myDevice, myRenderPass, {view, myDepthImageView}, windowWidth, windowHeight, fb);
		assert(boolResult);
		myFramebuffersVector.push_back(fb);
	}

	// Create a buffer to use as the vertex buffer.
	const size_t vertexBufferSize = sizeof(TriangleDemoVertex)*NUM_DEMO_VERTICES;
	VkBuffer myVertexBuffer;
	VkDeviceMemory myVertexBufferMemory;
	boolResult = vkdemos::createAndAllocateBuffer(myDevice,
	                                     myMemoryProperties,
	                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                                     vertexBufferSize,
	                                     myVertexBuffer,
	                                     myVertexBufferMemory
	                                     );
	assert(boolResult);

	// Map vertex buffer and insert data
	{
		void *mappedBuffer;
		result = vkMapMemory(myDevice, myVertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &mappedBuffer);
		assert(result == VK_SUCCESS);

		memcpy(mappedBuffer, vertices, vertexBufferSize);

		vkUnmapMemory(myDevice, myVertexBufferMemory);
	}


	/*
	 * Create the pipeline.
	 *
	 * We specify a single Push Constant Range at offset 0 of length 4 bytes,
	 * which we'll use to pass a single 32 bit float value to the shaders.
	 *
	 * Drivers are required to allow at least 128 bytes of push constants;
	 * if your push data is bigger than this, you should check that the
	 * physical device supports the bigger size with vkGetPhysicalDeviceProperties.
	 */
	const VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,    // We pass a single, 32-bit float value;
		.size = 4       // both "offset" and "size" are specified in bytes.
	};

	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};

	VkPipelineLayout myPipelineLayout;
	result = vkCreatePipelineLayout(myDevice, &pipelineLayoutCreateInfo, nullptr, &myPipelineLayout);
	assert(result == VK_SUCCESS);

	VkPipeline myGraphicsPipeline;
	boolResult = demo02CreatePipeline(myDevice, myRenderPass, myPipelineLayout, VERTEX_SHADER_FILENAME, FRAGMENT_SHADER_FILENAME, VERTEX_INPUT_BINDING, myGraphicsPipeline);
	assert(boolResult);


	/*
	 * Per-Frame data
	 */
	PerFrameData perFrameDataVector[FRAME_LAG];

	for(int i = 0; i < FRAME_LAG; i++)
	{
		boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, perFrameDataVector[i].presentCmdBuffer);
		assert(boolResult);

		result = vkdemos::utils::createFence(myDevice, perFrameDataVector[i].presentFence);
		assert(result == VK_SUCCESS);

		result = vkdemos::utils::createSemaphore(myDevice, perFrameDataVector[i].imageAcquiredSemaphore);
		assert(result == VK_SUCCESS);

		result = vkdemos::utils::createSemaphore(myDevice, perFrameDataVector[i].renderingCompletedSemaphore);
		assert(result == VK_SUCCESS);

		perFrameDataVector[i].fenceInitialized = false;
	}

	/*
	 * Generation and submission of the initialization commands' command buffer.
	 */
	// We fill the initialization command buffer with... the initialization commands.
	boolResult = demo02FillInitializationCommandBuffer(myCmdBufferInitialization, myDepthImage);
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
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);


	/*
	 * Event loop
	 */
	SDL_Event sdlEvent;
	bool quit = false;

	float animationTime = 0;

	// Just some variables for frame statistics
	long frameNumber = 0;
	long frameMaxTime = LONG_MIN;
	long frameMinTime = LONG_MAX;
	long frameAvgTimeSum = 0;
	long frameAvgTimeSumSquare = 0;
	constexpr long FRAMES_PER_STAT = 120;	// How many frames to wait before printing frame time statistics.

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
			// Render a single frame
			auto renderStartTime = std::chrono::high_resolution_clock::now();
			quit = !demo04RenderSingleFrame(myDevice, myQueue, mySwapchain, myFramebuffersVector, myRenderPass, myGraphicsPipeline, myPipelineLayout, myVertexBuffer, VERTEX_INPUT_BINDING, perFrameDataVector[frameNumber % FRAME_LAG], windowWidth, windowHeight, animationTime);
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

			// Update animation
			animationTime += elapsedTimeUs / 1000.0f;
		}
	}


	/*
	 * Deinitialization
	 */
	// We wait for pending operations to complete before starting to destroy stuff.
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);

	// Destroy the objects in the perFrameDataVector array.
	for(int i = 0; i < FRAME_LAG; i++)
	{
		vkDestroyFence(myDevice, perFrameDataVector[i].presentFence, nullptr);
		vkDestroySemaphore(myDevice, perFrameDataVector[i].imageAcquiredSemaphore, nullptr);
		vkDestroySemaphore(myDevice, perFrameDataVector[i].renderingCompletedSemaphore, nullptr);
	}

	/*
	 * For more informations on the following commands, refer to Demo 02.
	 */
	vkDestroyPipeline(myDevice, myGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(myDevice, myPipelineLayout, nullptr);
	vkDestroyBuffer(myDevice, myVertexBuffer, nullptr);
	vkFreeMemory(myDevice, myVertexBufferMemory, nullptr);

	for(auto framebuffer : myFramebuffersVector)
		vkDestroyFramebuffer(myDevice, framebuffer, nullptr);

	vkDestroyRenderPass(myDevice, myRenderPass, nullptr);
	vkDestroyImageView(myDevice, myDepthImageView, nullptr);
	vkDestroyImage(myDevice, myDepthImage, nullptr);
	vkFreeMemory(myDevice, myDepthMemory, nullptr);

	/*
	 * For more informations on the following commands, refer to Demo 01.
	 */
	vkDestroyCommandPool(myDevice, myCommandPool, nullptr);

	for(auto imgView : mySwapchainImageViewsVector)
		vkDestroyImageView(myDevice, imgView, nullptr);

	vkDestroySwapchainKHR(myDevice, mySwapchain, nullptr);
	vkDestroyDevice(myDevice, nullptr);
	vkDestroySurfaceKHR(myInstance, mySurface, nullptr);
	vkdemos::destroyDebugReportCallback(myInstance, myDebugReportCallback);
	vkDestroyInstance(myInstance, nullptr);

	SDL_DestroyWindow(mySdlWindow);
	SDL_Quit();

	return 0;
}
