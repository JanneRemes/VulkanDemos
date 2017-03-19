// Demo 06: Compute.

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
#include "../00_commons/10_submitimagebarrier.h"
#include "../00_commons/11_loadimagefromfile.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "../00_commons/glm/glm/glm.hpp"
#include "../00_commons/glm/glm/gtc/matrix_transform.hpp"

#include "demo06rendersingleframe.h"
#include "demo06createpipeline.h"
#include "demo06createcomputepipeline.h"
#include "demo06createvkdeviceandvkqueues.h"
#include "demo06computesinglestep.h"
#include "pushconstdata.h"

// CreateRenderPass are the same as Demo 02
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
#include <random>


/*
 * Constants
 */
static const int FRAME_LAG = 2;

static const std::string VERTEX_SHADER_FILENAME   = "vertex.spirv";
static const std::string FRAGMENT_SHADER_FILENAME = "fragment.spirv";
static const std::string COMPUTE_SHADER_FILENAME = "compute.spirv";

static constexpr int VERTEX_INPUT_BINDING = 0;

static constexpr int NUM_COMPUTE_STORAGE_IMAGES = 4;

static constexpr int FRAMES_PER_COMPUTE = 5;	// How many frames to render for every compute dispatch.


// Vertex data to draw.
static constexpr int NUM_DEMO_VERTICES = 3;
static const Demo06Vertex vertices[NUM_DEMO_VERTICES] =
{
	//   position
	{ -1.0f, -1.0f },
	{  3.0f, -1.0f },
    { -1.0f,  3.0f },
};

// Arena for Conway's Game of Life simulation
static constexpr int ARENA_WIDTH = 256;
static constexpr int ARENA_HEIGHT = 256;
uint8_t arenaInitialization[ARENA_WIDTH*ARENA_HEIGHT];



/**
 * Good ol' main function.
 */
int main(int argc, char* argv[])
{
	static int windowWidth = 512;
	static int windowHeight = 512;
	static const char * applicationName = "SdlVulkanDemo_06_compute";
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
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
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
	VkQueue myComputeQueue;
	uint32_t myComputeQueueFamilyIndex;
	boolResult = demo06createVkDeviceAndVkQueues(myPhysicalDevice, mySurface, layersNamesToEnable, myDevice, myQueue, myQueueFamilyIndex, myComputeQueue, myComputeQueueFamilyIndex);
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

	boolResult = vkdemos::createAndAllocateImage(
		myDevice,
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
	const size_t vertexBufferSize = sizeof(Demo06Vertex)*NUM_DEMO_VERTICES;
	VkBuffer myVertexBuffer;
	VkDeviceMemory myVertexBufferMemory;

	boolResult = vkdemos::createAndAllocateBuffer(
		myDevice,
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
	 * Create the arena buffers.
	 *
	 * We need an area of memory for the compute shader to
	 * compute the next state in the simulation; we do this by
	 * creating various Storage Images (spec. 13.1.1), so that
	 * we have at least one to use as the current state and
	 * one as the next state.
	 *
	 * To optimize memory allocation, we allocate a single
	 * memory area from the GPU, and then we create the various
	 * VkImage with different offsets in this area.
	 */
	VkImage myArenaStorageImages[NUM_COMPUTE_STORAGE_IMAGES];
	VkImageView myArenaStorageImagesViews[NUM_COMPUTE_STORAGE_IMAGES];
	VkDeviceMemory myArenaStorageImagesMemory;
	VkBuffer myArenaStagingBuffer;
	VkDeviceMemory myArenaStagingBufferMemory;

	{
		std::random_device rd;
	    std::mt19937 mt(rd());
	    std::uniform_int_distribution<int> dist(0, 1);

		for(int i = 0; i < ARENA_WIDTH*ARENA_HEIGHT; i++)
			arenaInitialization[i] = dist(mt);

		/*
		 * Create the VkImages
		 */
		uint32_t queueFamilyIndices[2] = {myQueueFamilyIndex, myComputeQueueFamilyIndex};

		const VkImageCreateInfo imageCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8_UINT,
			.extent = {(uint32_t)ARENA_WIDTH, (uint32_t)ARENA_HEIGHT, 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			.sharingMode = VK_SHARING_MODE_CONCURRENT,
			.queueFamilyIndexCount = 2,
			.pQueueFamilyIndices = queueFamilyIndices,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		for(int i = 0; i < NUM_COMPUTE_STORAGE_IMAGES; i++) {
			result = vkCreateImage(myDevice, &imageCreateInfo, nullptr, &myArenaStorageImages[i]);
			assert(result == VK_SUCCESS);
		}



		// Get the memory requirements for our images.
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(myDevice, myArenaStorageImages[0], &memoryRequirements);

		// Check if all the images have the same requirements
		// (This is more to shut the "vkGetImageMemoryRequirements() has not been called on that image" warning than anything)
		for(int i = 1; i < NUM_COMPUTE_STORAGE_IMAGES; i++)
		{
			VkMemoryRequirements vkmr;
			vkGetImageMemoryRequirements(myDevice, myArenaStorageImages[i], &vkmr);

			assert(
				memoryRequirements.size == vkmr.size &&
				memoryRequirements.alignment == vkmr.alignment &&
				memoryRequirements.memoryTypeBits == vkmr.memoryTypeBits
			);
		}

		// Find an appropriate memory type with all the requirements for our image.
		int memoryTypeIndex = vkdemos::utils::findMemoryTypeWithProperties(myMemoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if(memoryTypeIndex < 0) {
			std::cout << "!!! ERROR: Can't find a memory type to hold the image." << std::endl;
			return false;
		}


		// Allocate memory for the image.
		const VkMemoryAllocateInfo memoryAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memoryRequirements.size * NUM_COMPUTE_STORAGE_IMAGES,
			.memoryTypeIndex = (uint32_t)memoryTypeIndex,
		};

		result = vkAllocateMemory(myDevice, &memoryAllocateInfo, nullptr, &myArenaStorageImagesMemory);
		assert(result == VK_SUCCESS);


		/*
		 * Bind the allocated memory to the images
		 */
		for(int i = 0; i < NUM_COMPUTE_STORAGE_IMAGES; i++)
		{
			// Bind memory
			result = vkBindImageMemory(myDevice, myArenaStorageImages[i], myArenaStorageImagesMemory, memoryRequirements.size*i);
			assert(result == VK_SUCCESS);

			// Create view
			const VkImageViewCreateInfo imageViewCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = myArenaStorageImages[i],
				.format = VK_FORMAT_R8_UINT,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
			};

			result = vkCreateImageView(myDevice, &imageViewCreateInfo, nullptr, &myArenaStorageImagesViews[i]);
			assert(result == VK_SUCCESS);
		}


		/*
		 * As for Demo 05, we use a staging buffer to copy the initialization
		 * data for the first iteration of the simulation.
		 */
		boolResult = vkdemos::createAndAllocateBuffer(
			myDevice,
			myMemoryProperties,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			ARENA_WIDTH*ARENA_HEIGHT*sizeof(uint8_t),
			myArenaStagingBuffer,
			myArenaStagingBufferMemory
		);
		assert(boolResult);

		// Map the buffer and fill it with data.
		void *mappedBuffer;
		result = vkMapMemory(myDevice, myArenaStagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &mappedBuffer);
		assert(result == VK_SUCCESS);

		memcpy(mappedBuffer, reinterpret_cast<const unsigned char *>(arenaInitialization), ARENA_WIDTH*ARENA_HEIGHT*sizeof(uint8_t));

		VkMappedMemoryRange mappedMemoryRange = {
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.pNext = nullptr,
			.memory = myArenaStagingBufferMemory,
			.offset = 0,
			.size = VK_WHOLE_SIZE,
		};

		vkFlushMappedMemoryRanges(myDevice, 1, &mappedMemoryRange);
		vkUnmapMemory(myDevice, myArenaStagingBufferMemory);


		/*
		 * Submit a buffer->image copy command to the GPU.
		 */
		VkCommandBuffer textureCopyCmdBuffer;
		boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, textureCopyCmdBuffer);
		assert(boolResult);

		// Begin command buffer recording.
		VkCommandBufferBeginInfo commandBufferBeginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = (VkCommandBufferUsageFlags)0,
			.pInheritanceInfo = nullptr,
		};

		result = vkBeginCommandBuffer(textureCopyCmdBuffer, &commandBufferBeginInfo);
		assert(result == VK_SUCCESS);


		/*
		 * Transition image to an optimal layout for transfer
		 */
		{
			std::vector<VkImageMemoryBarrier> imageMemoryBarrierVector;

			VkImageMemoryBarrier imageMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.subresourceRange =  {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
				.image = myArenaStorageImages[0],  // Initialize only the first image
			};

			imageMemoryBarrierVector.push_back(imageMemoryBarrier);

			// While I'm at it, transition all the other storage images from
			// VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_GENERAL.

			for(int i = 1; i < NUM_COMPUTE_STORAGE_IMAGES; i++) {
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.image = myArenaStorageImages[i];
				imageMemoryBarrierVector.push_back(imageMemoryBarrier);
			}

			// Barrier for the buffer, to synchronize with the host upload
			VkBufferMemoryBarrier bufferMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = myArenaStagingBuffer,
				.offset = 0,
				.size = VK_WHOLE_SIZE,
			};

			vkCmdPipelineBarrier(textureCopyCmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // srcStageMask
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // dstStageMask
				0,                                 // dependencyFlags
				0,                                 // memoryBarrierCount
				nullptr,                           // pMemoryBarriers
				1,                                 // bufferMemoryBarrierCount
				&bufferMemoryBarrier,              // pBufferMemoryBarriers
				imageMemoryBarrierVector.size(),   // imageMemoryBarrierCount
				imageMemoryBarrierVector.data()    // pImageMemoryBarriers
			);
		}


		// Copy mip levels from staging buffer
		VkBufferImageCopy bufferImageCopy = {
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
		    },
		    .imageOffset = {0, 0, 0},
		    .imageExtent = {.width = ARENA_WIDTH, .height = ARENA_HEIGHT, .depth = 1},
		};

		vkCmdCopyBufferToImage(
			textureCopyCmdBuffer,
			myArenaStagingBuffer,
			myArenaStorageImages[0],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferImageCopy
		);

		// Transition the Image to an optimal layout for use as a shader's read-only sampling source.
		vkdemos::submitImageBarrier(
			textureCopyCmdBuffer,
			myArenaStorageImages[0],
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
		    {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		);

		// End command buffer recording.
		result = vkEndCommandBuffer(textureCopyCmdBuffer);

		// Submit the command buffer to the queue
		VkSubmitInfo submitInfo = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &textureCopyCmdBuffer,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		result = vkQueueSubmit(myQueue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(result == VK_SUCCESS);
	}


	/*
	 * Specify Push Constant parameters.
	 */
	static_assert(sizeof(PushConstData) % 4 == 0, "PushConstData size is not a multiple of 4 bytes.");

	const VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,                   // We start at offset 0,
		.size = sizeof(PushConstData)  // both "offset" and "size" are specified in bytes, and must be multiple of 4.
	};


	/*
	 * Create descriptor pool.
	 */
	VkDescriptorPoolSize descriptorPoolSize = {
	    .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	    .descriptorCount = NUM_COMPUTE_STORAGE_IMAGES * 2 + FRAME_LAG,
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .maxSets = NUM_COMPUTE_STORAGE_IMAGES + FRAME_LAG,
	    .poolSizeCount = 1,
	    .pPoolSizes = &descriptorPoolSize,
	};

	VkDescriptorPool myDescriptorPool;
	result = vkCreateDescriptorPool(myDevice, &descriptorPoolCreateInfo, nullptr, &myDescriptorPool);
	assert(result == VK_SUCCESS);


	/*
	 * Create the Graphics descriptor set and pipeline.
	 */
	VkDescriptorSetLayout myGraphicsDescriptorSetLayout;
	VkPipelineLayout myGraphicsPipelineLayout;
	VkPipeline myGraphicsPipeline;

	{
		VkDescriptorSetLayoutBinding graphicsDescriptorSetLayoutBinding =
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutCreateInfo graphicsDescriptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 1,
			.pBindings = &graphicsDescriptorSetLayoutBinding,
		};

		result = vkCreateDescriptorSetLayout(myDevice, &graphicsDescriptorSetLayoutCreateInfo, nullptr, &myGraphicsDescriptorSetLayout);
		assert(result == VK_SUCCESS);

		// Create pipeline
		const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &myGraphicsDescriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange,
		};

		result = vkCreatePipelineLayout(myDevice, &pipelineLayoutCreateInfo, nullptr, &myGraphicsPipelineLayout);
		assert(result == VK_SUCCESS);

		boolResult = demo06CreatePipeline(myDevice, myRenderPass, myGraphicsPipelineLayout, VERTEX_SHADER_FILENAME, FRAGMENT_SHADER_FILENAME, VERTEX_INPUT_BINDING, myGraphicsPipeline);
		assert(boolResult);
	}


	/*
	 * Create the Compute descriptor set and pipeline.
	 */
	VkDescriptorSetLayout myComputeDescriptorSetLayout;
	VkPipelineLayout myComputePipelineLayout;
	VkPipeline myComputePipeline;

	{
		VkDescriptorSetLayoutBinding computeDescriptorSetLayoutBindings[2] =
		{
			// "previousState"
			[0] = {
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr,
			},
			// "nextState"
			[1] = {
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.pImmutableSamplers = nullptr,
			},
		};

		VkDescriptorSetLayoutCreateInfo computeDescriptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.bindingCount = 2,
			.pBindings = computeDescriptorSetLayoutBindings,
		};

		result = vkCreateDescriptorSetLayout(myDevice, &computeDescriptorSetLayoutCreateInfo, nullptr, &myComputeDescriptorSetLayout);
		assert(result == VK_SUCCESS);

		// Create pipeline
		const VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &myComputeDescriptorSetLayout,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &pushConstantRange,
		};

		result = vkCreatePipelineLayout(myDevice, &computePipelineLayoutCreateInfo, nullptr, &myComputePipelineLayout);
		assert(result == VK_SUCCESS);

		boolResult = demo06CreateComputePipeline(myDevice, myComputePipelineLayout, COMPUTE_SHADER_FILENAME, myComputePipeline);
		assert(boolResult);
	}


	/*
	 * Allocate Descriptor Sets for Graphics.
	 */
	VkDescriptorSet myGraphicsDescriptorSets[FRAME_LAG];

	for(int i = 0; i < FRAME_LAG; i++)
	{
		VkDescriptorSetAllocateInfo graphicsDescriptorSetAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = myDescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &myGraphicsDescriptorSetLayout,
		};

		result = vkAllocateDescriptorSets(myDevice, &graphicsDescriptorSetAllocateInfo, &myGraphicsDescriptorSets[i]);
		assert(result == VK_SUCCESS);
	}


	/*
	 * Allocate Descriptor Sets for Compute.
	 */
	VkDescriptorSet myComputeDescriptorSets[NUM_COMPUTE_STORAGE_IMAGES];

	for(int i = 0; i < NUM_COMPUTE_STORAGE_IMAGES; i++)
	{
		VkDescriptorSetAllocateInfo computeDescriptorSetAllocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = myDescriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &myComputeDescriptorSetLayout,
		};

		result = vkAllocateDescriptorSets(myDevice, &computeDescriptorSetAllocateInfo, &myComputeDescriptorSets[i]);
		assert(result == VK_SUCCESS);
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

	// Per-Frame and per-compute data.
	PerFrameData perFrameDataVector[FRAME_LAG];
	PerComputeData perComputeDataVector[NUM_COMPUTE_STORAGE_IMAGES];

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

	for(int i = 0; i < NUM_COMPUTE_STORAGE_IMAGES; i++)
	{
		boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, perComputeDataVector[i].computeCmdBuffer);
		assert(boolResult);

		result = vkdemos::utils::createFence(myDevice, perComputeDataVector[i].computeFence);
		assert(result == VK_SUCCESS);

		result = vkdemos::utils::createSemaphore(myDevice, perComputeDataVector[i].computeCompletedSemaphore);
		assert(result == VK_SUCCESS);

		perComputeDataVector[i].fenceInitialized = false;
	}

	// Wait for the queue to complete its work.
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);


	std::cout << "--- Rendering start ---" << std::endl;


	/*
	 * Event loop
	 */
	SDL_Event sdlEvent;
	bool quit = false, quit2 = false;

	PushConstData pushConstData;
	pushConstData.windowSize = {windowWidth, windowHeight};
	pushConstData.arenaSize = {ARENA_WIDTH, ARENA_HEIGHT};

	int mostRecentlyUpdatedArenaImageIndex = 0;
	VkImageView mostRecentlyUpdatedArenaImageView = myArenaStorageImagesViews[0];
	VkSemaphore computeSemaphoreToWait = VK_NULL_HANDLE;

	// Just some variables for frame statistics
	long frameNumber = 0;
	long frameMaxTime = LONG_MIN;
	long frameMinTime = LONG_MAX;
	long frameAvgTimeSum = 0;
	long frameAvgTimeSumSquare = 0;
	constexpr long FRAMES_PER_STAT = 120;	// How many frames to wait before printing frame time statistics.


	// The main event/render loop.
	while(!quit && !quit2)
	{
		// Process events for this frame
		while(!quit && SDL_PollEvent(&sdlEvent))
		{
			if(sdlEvent.type == SDL_QUIT) {
				quit = true;
			}
			if(sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
				quit = true;
			}
		}


		// Rendering code
		if(!quit)
		{
			PerFrameData & perFrameData = perFrameDataVector[frameNumber % FRAME_LAG];
			const VkDescriptorSet & activeGraphicsDescriptorSet = myGraphicsDescriptorSets[frameNumber % FRAME_LAG];

			// Render a single frame
			auto renderStartTime = std::chrono::high_resolution_clock::now();


			/*
			 * Start dispatching the compute job: we run it only every Nth frame,
			 * so that our simulation is slow enough for us to see.
			 */
			computeSemaphoreToWait = VK_NULL_HANDLE;
			if(frameNumber % FRAMES_PER_COMPUTE == 0)
			{
				VkImageView currentlyUpdatedArenaImageView = mostRecentlyUpdatedArenaImageView;

				mostRecentlyUpdatedArenaImageIndex = (mostRecentlyUpdatedArenaImageIndex + 1) % NUM_COMPUTE_STORAGE_IMAGES;
				mostRecentlyUpdatedArenaImageView = myArenaStorageImagesViews[mostRecentlyUpdatedArenaImageIndex];

				PerComputeData & perComputeData = perComputeDataVector[mostRecentlyUpdatedArenaImageIndex];
				const VkDescriptorSet & activeComputeDescriptorSet = myComputeDescriptorSets[mostRecentlyUpdatedArenaImageIndex];


				// Update compute descriptor set
				{
					VkDescriptorImageInfo descriptorImageInfos[2] =
					{
					    [0] = {
							.sampler = VK_NULL_HANDLE,
							.imageView = currentlyUpdatedArenaImageView,
							.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					    },
					    [1] = {
							.sampler = VK_NULL_HANDLE,
							.imageView = mostRecentlyUpdatedArenaImageView,
							.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					    },
					};

					VkWriteDescriptorSet writeDescriptorSets[2] = {
					    [0] = {
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.pNext = nullptr,
							.dstSet = activeComputeDescriptorSet,
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
							.pImageInfo = &descriptorImageInfos[0],
							.pBufferInfo = nullptr,
							.pTexelBufferView = nullptr,
					    },
					    [1] = {
					        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.pNext = nullptr,
							.dstSet = activeComputeDescriptorSet,
							.dstBinding = 1,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
							.pImageInfo = &descriptorImageInfos[1],
							.pBufferInfo = nullptr,
							.pTexelBufferView = nullptr,
					    }
					};

					vkUpdateDescriptorSets(myDevice, 2, writeDescriptorSets, 0, nullptr);
				}

				quit = !demo06ComputeSingleStep(myDevice, myComputeQueue, myComputePipeline, myComputePipelineLayout, activeComputeDescriptorSet, perComputeData, ARENA_WIDTH, ARENA_HEIGHT, pushConstData);
				if(quit) break;

				computeSemaphoreToWait = perComputeData.computeCompletedSemaphore;
			}


			/*
			 * Now submit the rendering commands; we pass the compute semaphore's handle
			 * so that the graphics queue can correctly wait for the results before rendering.
			 */
			// Wait for the frame's fence
			if(perFrameData.fenceInitialized) {
				vkWaitForFences(myDevice, 1, &perFrameData.presentFence, VK_TRUE, UINT64_MAX);
				vkResetFences(myDevice, 1, &perFrameData.presentFence);
			}

			// Update the graphics descriptor set: tell the fragment shader which storage image to use
			// to draw the arena grid.
			{
				VkDescriptorImageInfo descriptorImageInfo = {
					.sampler = VK_NULL_HANDLE,		// ignored for VK_DESCRIPTOR_TYPE_STORAGE_IMAGE (Spec. 13.2.4)
					.imageView = mostRecentlyUpdatedArenaImageView,
					.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
				};

				VkWriteDescriptorSet writeDescriptorSet = {
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = activeGraphicsDescriptorSet,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.pImageInfo = &descriptorImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr,
				};

				vkUpdateDescriptorSets(myDevice, 1, &writeDescriptorSet, 0, nullptr);
			}

			quit = !demo06RenderSingleFrame(
				myDevice,
				myQueue,
				mySwapchain,
				myFramebuffersVector,
				myRenderPass,
				myGraphicsPipeline,
				myGraphicsPipelineLayout,
				myVertexBuffer,
				VERTEX_INPUT_BINDING,
				NUM_DEMO_VERTICES,
				activeGraphicsDescriptorSet,
				computeSemaphoreToWait,
				perFrameData,
				windowWidth,
				windowHeight,
				pushConstData
			);


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
				          << " ("<< 1000000.0f/average << " FPS)"
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

	for(int i = 0; i < FRAME_LAG; i++)
	{
		vkDestroyFence(myDevice, perFrameDataVector[i].presentFence, nullptr);
		vkDestroySemaphore(myDevice, perFrameDataVector[i].imageAcquiredSemaphore, nullptr);
		vkDestroySemaphore(myDevice, perFrameDataVector[i].renderingCompletedSemaphore, nullptr);
	}

	for(int i = 0; i < NUM_COMPUTE_STORAGE_IMAGES; i++)
	{
		vkDestroyFence(myDevice, perComputeDataVector[i].computeFence, nullptr);
		vkDestroySemaphore(myDevice, perComputeDataVector[i].computeCompletedSemaphore, nullptr);
	}

	// Destroy descriptor pool/set layout
	vkDestroyDescriptorPool(myDevice, myDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(myDevice, myGraphicsDescriptorSetLayout, nullptr);
	vkDestroyDescriptorSetLayout(myDevice, myComputeDescriptorSetLayout, nullptr);

	// Free the arena storage images and staging buffer
	for(int i = 0; i < NUM_COMPUTE_STORAGE_IMAGES; i++) {
		vkDestroyImageView(myDevice, myArenaStorageImagesViews[i], nullptr);
		vkDestroyImage(myDevice, myArenaStorageImages[i], nullptr);
	}
	vkFreeMemory(myDevice, myArenaStorageImagesMemory, nullptr);

	vkDestroyBuffer(myDevice, myArenaStagingBuffer, nullptr);
	vkFreeMemory(myDevice, myArenaStagingBufferMemory, nullptr);

	// For more informations on the following commands, refer to Demo 02.
	vkDestroyPipeline(myDevice, myComputePipeline, nullptr);
	vkDestroyPipelineLayout(myDevice, myComputePipelineLayout, nullptr);
	vkDestroyPipeline(myDevice, myGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(myDevice, myGraphicsPipelineLayout, nullptr);
	vkDestroyBuffer(myDevice, myVertexBuffer, nullptr);
	vkFreeMemory(myDevice, myVertexBufferMemory, nullptr);

	for(auto framebuffer : myFramebuffersVector)
		vkDestroyFramebuffer(myDevice, framebuffer, nullptr);

	vkDestroyRenderPass(myDevice, myRenderPass, nullptr);
	vkDestroyImageView(myDevice, myDepthImageView, nullptr);
	vkDestroyImage(myDevice, myDepthImage, nullptr);
	vkFreeMemory(myDevice, myDepthMemory, nullptr);

	// For more informations on the following commands, refer to Demo 01.
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
