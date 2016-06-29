// Demo 05: Textures.

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

#include "demo05rendersingleframe.h"
#include "demo05createpipeline.h"
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


/*
 * Constants
 */
static const int FRAME_LAG = 2;

static const std::string VERTEX_SHADER_FILENAME   = "vertex.spirv";
static const std::string FRAGMENT_SHADER_FILENAME = "fragment.spirv";

static constexpr int VERTEX_INPUT_BINDING = 0;


// Vertex data to draw.
static constexpr int NUM_DEMO_VERTICES = 3*2*6;
static const Demo05Vertex vertices[NUM_DEMO_VERTICES] =
{
	//      position       |     uv
	{ -0.5f, -0.5f,  0.5f,  0.0f, 0.0f },        // front
	{  0.5f,  0.5f,  0.5f,  1.0f, 1.0f },
	{ -0.5f,  0.5f,  0.5f,  0.0f, 1.0f },

	{ -0.5f, -0.5f,  0.5f,  0.0f, 0.0f },
	{  0.5f, -0.5f,  0.5f,  1.0f, 0.0f },
	{  0.5f,  0.5f,  0.5f,  1.0f, 1.0f },

	{ -0.5f, -0.5f, -0.5f,  1.0f, 0.0f },        // back
	{ -0.5f,  0.5f, -0.5f,  1.0f, 1.0f },
	{  0.5f,  0.5f, -0.5f,  0.0f, 1.0f },

	{ -0.5f, -0.5f, -0.5f,  1.0f, 0.0f },
	{  0.5f,  0.5f, -0.5f,  0.0f, 1.0f },
	{  0.5f, -0.5f, -0.5f,  0.0f, 0.0f },

	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f },        // left
	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f },
	{ -0.5f, -0.5f, -0.5f,  0.0f, 0.0f },

	{ -0.5f, -0.5f,  0.5f,  1.0f, 0.0f },
	{ -0.5f,  0.5f,  0.5f,  1.0f, 1.0f },
	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f },

	{  0.5f, -0.5f,  0.5f,  0.0f, 0.0f },        // right
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f },
	{  0.5f,  0.5f, -0.5f,  1.0f, 1.0f },

	{  0.5f, -0.5f,  0.5f,  0.0f, 0.0f },
	{  0.5f,  0.5f, -0.5f,  1.0f, 1.0f },
	{  0.5f,  0.5f,  0.5f,  0.0f, 1.0f },

	{ -0.5f, -0.5f,  0.5f,  0.0f, 1.0f },        // top
	{ -0.5f, -0.5f, -0.5f,  0.0f, 0.0f },
	{  0.5f, -0.5f,  0.5f,  1.0f, 1.0f },

	{ -0.5f, -0.5f, -0.5f,  0.0f, 0.0f },
	{  0.5f, -0.5f, -0.5f,  1.0f, 0.0f },
	{  0.5f, -0.5f,  0.5f,  1.0f, 1.0f },

	{ -0.5f,  0.5f,  0.5f,  0.0f, 0.0f },        // bottom
	{  0.5f,  0.5f,  0.5f,  1.0f, 0.0f },
	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f },

	{ -0.5f,  0.5f, -0.5f,  0.0f, 1.0f },
	{  0.5f,  0.5f,  0.5f,  1.0f, 0.0f },
	{  0.5f,  0.5f, -0.5f,  1.0f, 1.0f },
};


// Texture stuff
struct PixelData {    // data for a single pixel.
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;
};

static constexpr int TEXTURE_WIDTH = 256;
static constexpr int TEXTURE_HEIGHT = 256;
static constexpr const char* TEXTURE_FILE_NAME = "texture.png";


/**
 * Good ol' main function.
 */
int main(int argc, char* argv[])
{
	static int windowWidth = 600;
	static int windowHeight = 600;
	static const char * applicationName = "SdlVulkanDemo_05_textures";
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
	const size_t vertexBufferSize = sizeof(Demo05Vertex)*NUM_DEMO_VERTICES;
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
	 *
	 * Create the texture.
	 *
	 */
	VkBuffer myStagingBuffer;
	VkDeviceMemory myStagingBufferMemory;
	VkImage myTextureImage;
	VkImageView myTextureImageView;
	VkDeviceMemory myTextureImageMemory;

	{
		SDL_Surface* image = loadImageFromFile(TEXTURE_FILE_NAME);
		assert(image != nullptr && image->pixels != nullptr);
		assert(image->w == TEXTURE_WIDTH && image->h == TEXTURE_HEIGHT);

		/*
		 * Allocate memory for staging buffer, map it, and fill it with our texture's data.
		 */
		boolResult = vkdemos::createAndAllocateBuffer(
		                 myDevice,
		                 myMemoryProperties,
		                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Use this buffer as a source for transfers
		                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,                                 // It must be host-visible since we'll map it and copy data to it.
		                 TEXTURE_WIDTH*TEXTURE_HEIGHT*sizeof(PixelData),
		                 myStagingBuffer,
		                 myStagingBufferMemory
		             );
		assert(boolResult);

		// Map the buffer and fill it with data.
		void *mappedBuffer;
		result = vkMapMemory(myDevice, myStagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &mappedBuffer);
		assert(result == VK_SUCCESS);

		memcpy(mappedBuffer, reinterpret_cast<unsigned char *>(image->pixels), TEXTURE_WIDTH*TEXTURE_HEIGHT*sizeof(PixelData));

		vkUnmapMemory(myDevice, myStagingBufferMemory);



		/*
		 * Allocate memory for our texture's Image
		 */
		boolResult = vkdemos::createAndAllocateImage(
		                 myDevice,
		                 myMemoryProperties,
		                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,   // The image will be used as a sampling source and a transfer destination
		                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,    // The Image will reside in device-local memory.
		                 VK_FORMAT_R8G8B8A8_UNORM,
		                 TEXTURE_WIDTH,  // width
		                 TEXTURE_HEIGHT,  // height
		                 myTextureImage,
		                 myTextureImageMemory,
		                 &myTextureImageView,
		                 VK_IMAGE_ASPECT_COLOR_BIT
		             );
		assert(boolResult);


		/*
		 * Submit a copy command to the GPU to copy data from the staging buffer to the GPU's memory,
		 * with the appropriate format conversion.
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
		 * Transition the Image to an optimal layout for use as a copy command's destination;
		 * also set a memory barrier on the buffer such that the transfer of data is guaranteed
		 * to be completed before using it as a source for the copy.
		 * We can do both operations with a single Pipeline Barrier.
		 */
		{
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
				.image = myTextureImage,
			};

			VkBufferMemoryBarrier bufferMemoryBarrier = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = myStagingBuffer,
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
				1,                                 // imageMemoryBarrierCount
				&imageMemoryBarrier                // pImageMemoryBarriers
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
		    .imageExtent = {.width = TEXTURE_WIDTH, .height = TEXTURE_HEIGHT, .depth = 0},
		};

		vkCmdCopyBufferToImage(
			textureCopyCmdBuffer,
			myStagingBuffer,
			myTextureImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferImageCopy
		);

		// Transition the Image to an optimal layout for use as a shader's read-only sampling source.
		vkdemos::submitImageBarrier(
			textureCopyCmdBuffer,
			myTextureImage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
	 * Create the sampler.
	 *
	 * A sampler object contains all the parameters used to describe how
	 * the textures are sampled by the shaders.
	 * In OpenGL, sampler information was contained inside the texture object,
	 * but in Vulkan the two are separate entities, so you can sample many images
	 * with the same sampler, and you can use many samplers to sample the same image.
	 */
	VkSampler mySampler;

	VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,   // Anisotropic filter
		.maxAnisotropy = 8,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_NEVER,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE,
	};

	result = vkCreateSampler(myDevice, &samplerCreateInfo, nullptr, &mySampler);
	assert(result == VK_SUCCESS);


	/*
	 * Create the descriptor set layout.
	 *
	 * Note on field pImmutableSamplers: if you don't need to change your samplers
	 * at draw time, you can set the samplers now and you won't need to set them
	 * at draw time. Refer to section 13.2.1 of Vulkan's specification for more informations.
	 */
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr,
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &descriptorSetLayoutBinding,
	};

	VkDescriptorSetLayout myDescriptorSetLayout;
	result = vkCreateDescriptorSetLayout(myDevice, &descriptorSetLayoutCreateInfo, nullptr, &myDescriptorSetLayout);
	assert(result == VK_SUCCESS);



	/*
	 * Create descriptor pool.
	 */
	VkDescriptorPoolSize descriptorPoolSize = {
	    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    .descriptorCount = 1,
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .maxSets = 1,
	    .poolSizeCount = 1,
	    .pPoolSizes = &descriptorPoolSize,
	};

	VkDescriptorPool myDescriptorPool;
	result = vkCreateDescriptorPool(myDevice, &descriptorPoolCreateInfo, nullptr, &myDescriptorPool);
	assert(result == VK_SUCCESS);




	/*
	 * Create descriptor set.
	 */
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
	    .pNext = nullptr,
	    .descriptorPool = myDescriptorPool,
	    .descriptorSetCount = 1,
	    .pSetLayouts = &myDescriptorSetLayout,
	};

	VkDescriptorSet myDescriptorSet;
	result = vkAllocateDescriptorSets(myDevice, &descriptorSetAllocateInfo, &myDescriptorSet);
	assert(result == VK_SUCCESS);


	/*
	 * Update the descriptor set.
	 */
	VkDescriptorImageInfo descriptorImageInfo = {
	    .sampler = mySampler,
	    .imageView = myTextureImageView,
	    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkWriteDescriptorSet writeDescriptorSet = {
	    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	    .pNext = nullptr,
	    .dstSet = myDescriptorSet,
	    .dstBinding = 0,
	    .dstArrayElement = 0,
	    .descriptorCount = 1,
	    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	    .pImageInfo = &descriptorImageInfo,
	    .pBufferInfo = nullptr,
	    .pTexelBufferView = nullptr,
	};

	vkUpdateDescriptorSets(myDevice, 1, &writeDescriptorSet, 0, nullptr);



	/*
	 * Specify Push Constant parameters.
	 */
	static_assert(sizeof(PushConstData) % 4 == 0, "PushConstData size is not a multiple of 4 bytes.");

	const VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,                   // We start at offset 0,
		.size = sizeof(PushConstData)  // both "offset" and "size" are specified in bytes, and must be multiple of 4.
	};

	/*
	 * Create the pipeline.
	 */
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &myDescriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &pushConstantRange,
	};

	VkPipelineLayout myPipelineLayout;
	result = vkCreatePipelineLayout(myDevice, &pipelineLayoutCreateInfo, nullptr, &myPipelineLayout);
	assert(result == VK_SUCCESS);

	VkPipeline myPipeline;
	boolResult = demo05CreatePipeline(myDevice, myRenderPass, myPipelineLayout, VERTEX_SHADER_FILENAME, FRAGMENT_SHADER_FILENAME, VERTEX_INPUT_BINDING, myPipeline);
	assert(boolResult);


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

	// Per-Frame data.
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

	// Wait for the queue to complete its work.
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);

	/*
	 * Event loop
	 */
	SDL_Event sdlEvent;
	bool quit = false;

	PushConstData pushConstData;

	// Just some variables for frame statistics
	long frameNumber = 0;
	long frameMaxTime = LONG_MIN;
	long frameMinTime = LONG_MAX;
	long frameAvgTimeSum = 0;
	long frameAvgTimeSumSquare = 0;
	constexpr long FRAMES_PER_STAT = 120;	// How many frames to wait before printing frame time statistics.

	bool clicked = false;
	glm::ivec2 clickedMousePos;
	glm::vec2 cubeRotation{-30.0f, 0.0f};
	glm::vec2 cubeRotationAtClick;

	// The main event/render loop.
	while(!quit)
	{
		// Process events for this frame
		while(SDL_PollEvent(&sdlEvent))
		{
			if(sdlEvent.type == SDL_QUIT) {
				quit = true;
			}
			if(sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
				quit = true;
			}

			if(sdlEvent.type == SDL_MOUSEBUTTONDOWN && sdlEvent.button.button == SDL_BUTTON_LEFT) {
				clicked = true;
				clickedMousePos.x = sdlEvent.button.x;
				clickedMousePos.y = sdlEvent.button.y;
				cubeRotationAtClick = cubeRotation;
			}

			if(sdlEvent.type == SDL_MOUSEBUTTONUP && sdlEvent.button.button == SDL_BUTTON_LEFT) {
				clicked = false;
			}

			if(sdlEvent.type == SDL_MOUSEMOTION && clicked)
			{
				cubeRotation.y = cubeRotationAtClick.y + 200.0f*(sdlEvent.motion.x - clickedMousePos.x)/windowWidth;
				cubeRotation.x = cubeRotationAtClick.x - 200.0f*float(sdlEvent.motion.y - clickedMousePos.y)/windowHeight;
			}
		}


		// Rendering code
		if(!quit)
		{
			float animatedRotation = glm::mod(pushConstData.animationTime / 60.0f, 360.0f);

			// Calculate projection*model matrix.
			glm::mat4 projMatrix = glm::perspective(glm::radians(60.0f), (float)windowWidth / (float)windowHeight, 0.001f, 256.0f);
			glm::mat4 modelMatrix{};

			modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, -2.5f));
			modelMatrix = glm::rotate(modelMatrix, glm::radians(cubeRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			modelMatrix = glm::rotate(modelMatrix, glm::radians(cubeRotation.y+animatedRotation), glm::vec3(0.0f, 1.0f, 0.0f));

			pushConstData.projMatrix = projMatrix * modelMatrix;

			// Render a single frame
			auto renderStartTime = std::chrono::high_resolution_clock::now();
			quit = !demo05RenderSingleFrame(myDevice, myQueue, mySwapchain, myFramebuffersVector, myRenderPass, myPipeline, myPipelineLayout, myVertexBuffer, VERTEX_INPUT_BINDING, NUM_DEMO_VERTICES, myDescriptorSet, perFrameDataVector[frameNumber % FRAME_LAG], windowWidth, windowHeight, pushConstData);
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

			// Update animation
			pushConstData.animationTime += elapsedTimeUs / 1000.0f;
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

	// Destroy sampler and descriptor pool/set layout
	vkDestroyDescriptorPool(myDevice, myDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(myDevice, myDescriptorSetLayout, nullptr);
	vkDestroySampler(myDevice, mySampler, nullptr);

	// Free the staging buffer and the texture image.
	vkFreeMemory(myDevice, myStagingBufferMemory, nullptr);
	vkDestroyBuffer(myDevice, myStagingBuffer, nullptr);

	vkDestroyImageView(myDevice, myTextureImageView, nullptr);
	vkDestroyImage(myDevice, myTextureImage, nullptr);
	vkFreeMemory(myDevice, myTextureImageMemory, nullptr);

	// For more informations on the following commands, refer to Demo 02.
	vkDestroyPipeline(myDevice, myPipeline, nullptr);
	vkDestroyPipelineLayout(myDevice, myPipelineLayout, nullptr);
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
