// Demo 02: Triangle.

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
#include <cstddef>

#include <fstream>


/*
 * Structures
 */
struct TriangleDemoVertex {
	float x, y, z;
	float r, g, b;
};


/*
 * Constants
 */
static const std::string VERTEX_SHADER_FILENAME   = "vertex.spirv";
static const std::string FRAGMENT_SHADER_FILENAME = "fragment.spirv";

static constexpr int VERTEX_INPUT_BINDING = 0;	// The Vertex Input Binding for our vertex buffer.

// Vertex data to draw.
static constexpr int NUM_DEMO_VERTICES = 3;
static const TriangleDemoVertex vertices[NUM_DEMO_VERTICES] =
{
	//      position             color
	{  0.5f,  0.5f,  0.0f,  0.1f, 0.8f, 0.1f },
	{ -0.5f,  0.5f,  0.0f,  0.8f, 0.1f, 0.1f },
	{  0.0f, -0.5f,  0.0f,  0.1f, 0.1f, 0.8f },
};


/*
 * Prototypes for functions defined in this file.
 */
bool fillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer, const std::vector<VkImage> & theSwapchainImagesVector, const VkImage theDepthImage);
bool fillRenderingCommandBuffer(const VkCommandBuffer theCommandBuffer,
                                const VkFramebuffer theCurrentFramebuffer,
                                const VkRenderPass theRenderPass,
                                const VkPipeline thePipeline,
                                const VkBuffer theVertexBuffer,
                                const int width,
                                const int height
                                );
bool renderSingleFrame(const VkDevice theDevice,
                       const VkQueue theQueue,
                       const VkSwapchainKHR theSwapchain,
                       const VkCommandBuffer thePresentCmdBuffer,
                       const std::vector<VkImage> & theSwapchainImagesVector,
                       const std::vector<VkFramebuffer> & theFramebuffersVector,
                       const VkRenderPass theRenderPass,
                       const VkPipeline thePipeline,
                       const VkBuffer theVertexBuffer,
                       const int width,
                       const int height,
                       const VkFence thePresentFence
                       );




/**
 * Creates a VkImage and allocates memory for it.
 * The image is created with layout VK_IMAGE_LAYOUT_UNDEFINED,
 * and must be transitioned to the appropriate layout.
 *
 * If outImageViewPtr is not nullptr, a VkImageView for the new image is created.
 */
bool createAndAllocateImage(const VkDevice theDevice,
                            const VkPhysicalDeviceMemoryProperties theMemoryProperties,
                            const VkBufferUsageFlags imageUsage,
                            const VkMemoryPropertyFlags requiredMemoryProperties,
                            const VkFormat theImageFormat,
                            const int width,
                            const int height,
                            VkImage & outImage,
                            VkDeviceMemory & outImageMemory,
                            VkImageView * outImageViewPtr = nullptr,
                            VkImageAspectFlags viewSubresourceAspectMask = 0
                            )
{
	VkResult result;
	VkImage myImage;
	VkImageView myImageView;
	VkDeviceMemory myImageMemory;

	/*
	 * Create the VkImage.
	 *
	 * When we create the VkImage object with vkCreateImage, we only create a
	 * CPU-side object, that contains all the metadata of the image, but
	 * no GPU memory is allocated for storing its contents.
	 */
	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = theImageFormat,
		.extent = {(uint32_t)width, (uint32_t)height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = imageUsage,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // access exclusive to a single queue family at a time
	    .queueFamilyIndexCount = 0,                // unused in sharing mode exclusive
	    .pQueueFamilyIndices = nullptr,            // unused in sharing mode exclusive
	    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	result = vkCreateImage(theDevice, &imageCreateInfo, nullptr, &myImage);
	assert(result == VK_SUCCESS);


	/*
	 * Allocate memory for the image.
	 *
	 * To allocate the necessary GPU memory to store the contents of our image,
	 * we must find an appropriate memory heap that supports all the requirements
	 * (alignment, GPU/CPU visiblity etc) that our image needs.
	 *
	 * With vkGetImageMemoryRequirements, we query the VkImage object we just created,
	 * so that we then know what type of memory heap to search for in the VkDevice.
	 */

	// Get the memory requirements for our image.
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(theDevice, myImage, &memoryRequirements);

	// Find an appropriate memory type with all the requirements for our image.
	int memoryTypeIndex = vkdemos::utils::findMemoryTypeWithProperties(theMemoryProperties, memoryRequirements.memoryTypeBits, requiredMemoryProperties);
	if(memoryTypeIndex < 0) {
		std::cout << "!!! ERROR: Can't find a memory type to hold the image." << std::endl;
		return false;
	}

	// Allocate memory for the image.
	const VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = (uint32_t)memoryTypeIndex,
	};

	result = vkAllocateMemory(theDevice, &memoryAllocateInfo, nullptr, &myImageMemory);
	assert(result == VK_SUCCESS);


	/*
	 * Bind the allocated memory to the image.
	 *
	 * After we created a VkImage, queried its memory requirements, and allocated
	 * some appropriate VkDeviceMemory, we can tell Vulkan to use that VkDeviceMemory
	 * as a storage area for our VkImage object.
	 * This connection is established with vkBindImageMemory.
	 *
	 * Note that you could allocate a VkDeviceMemory much larger than needed for a single
	 * VkImage: this way you can bind many VkImages (with the same memory requirements)
	 * on different offsets inside the VkDeviceMemory.
	 * This is done mainly to ammortize the cost of memory allocation (there is a potentially large
	 * space and time overhead in each memory allocation), or it can be used for advanced techniques
	 * such as memory aliasing.
	 * In these demoes we just allocate a new VkDeviceMemory for each single VkImage, for simplicity.
	 */
	result = vkBindImageMemory(theDevice, myImage, myImageMemory, 0);
	assert(result == VK_SUCCESS);

	outImage = myImage;
	outImageMemory = myImageMemory;

	/*
	 * Create a View for the image.
	 */
	if(outImageViewPtr != nullptr)
	{
		const VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = myImage,
			.format = theImageFormat,
			.subresourceRange = {
				.aspectMask = viewSubresourceAspectMask,
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

		result = vkCreateImageView(theDevice, &imageViewCreateInfo, nullptr, &myImageView);
		assert(result == VK_SUCCESS);

		*outImageViewPtr = myImageView;
	}

	return true;
}






/**
 * Creates a VkFramebuffer object from a set of VkImageViews.
 */
bool createFramebuffer(const VkDevice theDevice,
                       const VkRenderPass theRenderPass,
                       const std::vector<VkImageView> & theViewAttachmentsVector,
                       const int width,
                       const int height,
                       VkFramebuffer & outFramebuffer
                       )
{
	VkResult result;

	const VkFramebufferCreateInfo framebufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.renderPass = theRenderPass,
		.attachmentCount = (uint32_t)theViewAttachmentsVector.size(),
		.pAttachments = theViewAttachmentsVector.data(),
		.width = (uint32_t)width,
		.height = (uint32_t)height,
		.layers = 1,
	};

	VkFramebuffer myFramebuffer;
	result = vkCreateFramebuffer(theDevice, &framebufferCreateInfo, nullptr, &myFramebuffer);
	assert(result == VK_SUCCESS);

	outFramebuffer = myFramebuffer;
	return true;
}




/**
 * Create the renderpass for this demo.
 */
bool createTriangleDemoRenderPass(const VkDevice theDevice,
                                  const VkFormat theSwapchainImagesFormat,
                                  const VkFormat theDepthBufferFormat,
                                  VkRenderPass & outRenderPass)
{
	VkResult result;

	/*
	 * We need to specify all the attachments that will be used inside
	 * this renderpass.
	 * For each attachment we specify its format, the number of samples,
	 * what operations to do when reading and writing on the attachment,
	 * the behaviour of the stencil buffer, and the initial and final
	 * layout of the attachment image.
	 *
	 * A nice functionality of renderpasses is that ee can specify
	 * the initial layout of the attachment before it enters the render pass,
	 * and the layout we expect to see after the render pass finishes.
	 * This way the driver will insert the appropriate layout change operations for us,
	 * with all the correct barriers in place!
	 */
	const VkAttachmentDescription attachmentDescription[2] = {
		[0] = {
			.flags = 0,
			.format = theSwapchainImagesFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		[1] = {
			.flags = 0,
			.format = theDepthBufferFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};


	/*
	 * A renderpass is composed by one or more subpasses.
	 *
	 * Subpasses represent a part of the overall renderpass rendering:
	 * they read from a set of input attachments and write their results in
	 * one or more color attachments, and optionally a depth and a stencil attachment.
	 * They also take a list of attachments in the renderpass that will be preserved
	 * throughout the execution of the subpass; with these informations, the driver can
	 * schedule the execution of the subpasses' rendering operations in the best way possible
	 * because it knows the full dependency graph between the subpasses.
	 *
	 * A renderpass must have at least one subpass.
	 */

	// We need to tell the subpass what color attachment and what depth attachment
	// it must use: we do this with VkAttachmentReferences.
	const VkAttachmentReference colorAttachmentReference = {
		.attachment = 0,              // This is the index of the attachment in the attachmentDescription vector defined above.
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkAttachmentReference depthAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	// We define the subpass here
	const VkSubpassDescription subpassDescription = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.flags = 0,
		.inputAttachmentCount = 0,                             // No input attachments
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,                             // A single color output attachment
		.pColorAttachments = &colorAttachmentReference,
		.pResolveAttachments = nullptr,                        // No resolve attachments
		.pDepthStencilAttachment = &depthAttachmentReference,  // A depth/stencil attachment
		.preserveAttachmentCount = 0,                          // We don't have any other attachments to preserve
		.pPreserveAttachments = nullptr,
	};


	/*
	 * Create the VkRenderPass object.
	 *
	 * The dependencyCount and pDependencies fields define a list
	 * of additional dependencies between pairs of subpasses
	 * in this renderpass; basically it's like an additional
	 * pipeline barrier between subpasses.
	 */
	const VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.attachmentCount = 2,
		.pAttachments = attachmentDescription, // All the attachments needed for all the subpasses.
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,     // List of subpasses in this renderpass.
		.dependencyCount = 0,
		.pDependencies = nullptr,
	};

	VkRenderPass myRenderPass;
	result = vkCreateRenderPass(theDevice, &renderPassCreateInfo, nullptr, &myRenderPass);
	assert(result == VK_SUCCESS);

	outRenderPass = myRenderPass;
	return true;
}




/**
 * Creates a VkBuffer and allocates memory for it.
 *
 * This function is basically identical to createAndAllocateImage;
 * for an extended explanation of Vulkan's memory binding, refer
 * to createAndAllocateImage's implementation.
 */
bool createAndAllocateBuffer(const VkDevice theDevice,
                             const VkPhysicalDeviceMemoryProperties theMemoryProperties,
                             const VkBufferUsageFlags bufferUsage,
                             const VkMemoryPropertyFlags requiredMemoryProperties,
                             const VkDeviceSize bufferSize,
                             VkBuffer & outBuffer,
                             VkDeviceMemory & outBufferMemory
                             )
{
	VkResult result;
	VkBuffer myBuffer;
	VkDeviceMemory myBufferMemory;

	/*
	 * Create the VkBuffer object.
	 */
	const VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = bufferSize,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,  // access exclusive to a single queue family at a time
		.queueFamilyIndexCount = 0,                // unused in sharing mode exclusive
		.pQueueFamilyIndices = nullptr,            // unused in sharing mode exclusive
	};

	result = vkCreateBuffer(theDevice, &bufferCreateInfo, nullptr, &myBuffer);
	assert(result == VK_SUCCESS);

	// Get memory requirements for the buffer.
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(theDevice, myBuffer, &memoryRequirements);

	// Find an appropriate memory type with all the requirements for our buffer
	int memoryTypeIndex = vkdemos::utils::findMemoryTypeWithProperties(theMemoryProperties, memoryRequirements.memoryTypeBits, requiredMemoryProperties);
	if(memoryTypeIndex < 0) {
		std::cout << "!!! ERROR: Can't find a memory type to hold the buffer." << std::endl;
		return false;
	}

	/*
	 * Allocate memory for the buffer.
	 */
	const VkMemoryAllocateInfo memoryAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = (uint32_t)memoryTypeIndex,
	};

	result = vkAllocateMemory(theDevice, &memoryAllocateInfo, nullptr, &myBufferMemory);
	assert(result == VK_SUCCESS);

	/*
	 * Bind the allocated memory to the buffer.
	 */
	result = vkBindBufferMemory(theDevice, myBuffer, myBufferMemory, 0);
	assert(result == VK_SUCCESS);

	outBuffer = myBuffer;
	outBufferMemory = myBufferMemory;
	return true;
}




/**
 * Loads a SPIR-V shader from file in path "filename" and creates a VkShaderModule from it.
 */
bool loadAndCreateShaderModule(const VkDevice theDevice, const std::string & filename, VkShaderModule & outShaderModule)
{
	VkResult result;

	/*
	 * Read file into memory.
	 */
	std::ifstream inFile;
	inFile.open(filename, std::ios_base::binary | std::ios_base::ate);

	if(!inFile) {
		std::cout << "!!! ERROR: couldn't open shader file \"" << filename << "\" for reading." << std::endl;
		return false;
	}

	long fileSize = inFile.tellg();
	std::vector<char> fileContents((size_t)fileSize);

	inFile.seekg(0, std::ios::beg);
	bool readStat = bool(inFile.read(fileContents.data(), fileSize));
	inFile.close();

	if(!readStat) {
		std::cout << "!!! ERROR: couldn't read shader file \"" << filename << "\"." << std::endl;
		return false;
	}

	/*
	 * Create shader module.
	 */
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = fileContents.size(),
		.pCode = reinterpret_cast<uint32_t*>(fileContents.data()),
	};

	VkShaderModule myModule;
	result = vkCreateShaderModule(theDevice, &shaderModuleCreateInfo, nullptr, &myModule);
	assert(result == VK_SUCCESS);

	outShaderModule = myModule;
	return true;
}



/**
 * Create the VkPipeline for this demo.
 *
 * Pipelines are "the things that do stuff": they are a path
 * inside the GPU that vertex and fragment data follow
 * and they define standardized processing stages that transform said data;
 * VkPipeline objects define how those stages are configured.
 *
 * For this demo, we define a single graphics pipeline, with a vertex shader
 * and a fragment shader; there is no input data other than a vertex buffer,
 * and the pipeline outputs to an RGBA color attachment and a depth buffer.
 */
bool createTriangleDemoPipeline(const VkDevice theDevice,
                                const VkRenderPass theRenderPass,
                                const VkPipelineLayout thePipelineLayout,
                                VkPipeline & outPipeline
                                )
{
	VkResult result;

	/*
	 * Load the shaders and create the VkShaderModules.
	 */
	VkShaderModule vertexShaderModule, fragmentShaderModule;
	bool b1, b2;
	b1 = loadAndCreateShaderModule(theDevice, VERTEX_SHADER_FILENAME, vertexShaderModule);
	b2 = loadAndCreateShaderModule(theDevice, FRAGMENT_SHADER_FILENAME, fragmentShaderModule);

	if(!b1 || !b2) {
		std::cout << "!!! ERROR: couldn't create shader modules." << std::endl;
		return false;
	}


	/*
	 * Specify the pipeline's stages.
	 *
	 * In this demo we have 2 stages: the vertex shader stage, and the fragment shader stage
	 * (other optional stages are tessellation and geometry shaders).
	 */
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {
		[0] = {    // Vertex shader
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext  = nullptr,
			.flags  = 0,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName  = "main",               // Shader entry point name (i.e. the function that will be called when shader's execution starts)
			.pSpecializationInfo = nullptr,	// This field allows to specify constant values for constants in SPIR-V modules, before compiling the pipeline.
		},
		[1] = {    // Fragment shader
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext  = nullptr,
			.flags  = 0,
			.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName  = "main",
			.pSpecializationInfo = nullptr,
		},
	};


	/*
	 * Specify parameters for vertex input.
	 *
	 */
	// A Vertex Input Binding describes a binding point on the pipeline where a vertex buffer can be connected.
	VkVertexInputBindingDescription vertexInputBindingDescription = {
		.binding = VERTEX_INPUT_BINDING,
		.stride = sizeof(TriangleDemoVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	// Vertex Attribute Descriptions describe the format of the data expected on a buffer
	// attached to a particular Vertex Input Binding.
	// Here we have two Attribute Descriptions: a vector of 3 float elements for vertex position (x,y,z),
	// and a vector of 3 float elements for vertex color (r,g,b).
	VkVertexInputAttributeDescription vertexInputAttributeDescription[2] = {
		[0] = {
			.location = 0,
			.binding = VERTEX_INPUT_BINDING,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(TriangleDemoVertex, x),
		},
		[1] = {
			.location = 1,
			.binding = VERTEX_INPUT_BINDING,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(TriangleDemoVertex, r),
		},
	};

	// This will go in the Pipeline Create Info struct.
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = vertexInputAttributeDescription,
	};


	/*
	 * Specify parameters for input assembly.
	 *
	 * The Input Assembly stage is a fixed function stage in the pipeline
	 * that collects all the processed vertices and sends them to the rasterizer.
	 * We must tell it how to interpret those vertices: in this demo, our vertices
	 * describe a sequence of indipendent triangles.
	 */
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};


	/*
	 * Specify what parameters will be part of the dynamic state.
	 *
	 * Some of the pipeline's parameters are static and baked into the pipeline object
	 * at the instant of creation, but others can be chosen to be changed at a later time;
	 * those parameters are what constitute the "dynamic state" of the pipeline.
	 *
	 * You can choose what parameters to leave static and what to make dynamic;
	 * refer to Vulkan's specification to know what parameters can be made dynamic.
	 * Just be aware that dynamic states cannot be taken into consideration when creating
	 * the pipeline object, so there are less possibilities for the driver to apply various
	 * device-dependent optimizations.
	 */
	VkDynamicState dynamicStateEnables[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStateEnables,
	};


	/*
	 * Specify the viewport state.
	 *
	 * We'll use dynamic viewport and scissor state, so we specify nullptr
	 * for these values.
	 */
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = nullptr,
		.scissorCount = 1,     // Must match viewportCount
		.pScissors = nullptr,
	};


	/*
	 * Specifiy rasterization parameters.
	 *
	 * These parameters tell the rasterizer how to treat the polygons it receives,
	 * for example what the fill mode is (fill the triangles or draw only the borders),
	 * the culling mode (front or back face culling), etc.
	 */
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
		.lineWidth = 1,
	};


	/*
	 * Specify blending parameters.
	 *
	 * In this demo we don't enable blending; this means the fragments generated will overwrite
	 * whatever the framebuffer contains at that moment.
	 */

	// We need to create a VkPipelineColorBlendAttachmentState for each color attachment
	// we have on the subpass of the renderpass where this pipeline will be used.
	// (We have only one in this demo)
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable = VK_FALSE,   // Disable blending for this demo. All the other values in this struct are ignored except colorWriteMask.
		.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		//    ^^^  Which components of the fragments to write out to memory (in this case, all of them).
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,                    // unused in this demo
		.logicOp = VK_LOGIC_OP_CLEAR,                 // (ignored)
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},   // (ignored if blending not enabled)
	};


	/*
	 * Specify depth buffer and stencil buffer parameters.
	 *
	 * These parameters tell the GPU how to treat values in the depth and stencil buffers
	 * and what to do with with fragments that pass/fail the depth/stencil tests.
	 */
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {               // These values are ignored since stencilTestEnable is false
			.failOp = VK_STENCIL_OP_KEEP,
			.passOp = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.compareMask = 0,
			.writeMask = 0,
			.reference = 0,
		},
		.minDepthBounds = 0.0f,  // (ignored because depthBoundsTestEnable is false)
		.maxDepthBounds = 0.0f,  // (ignored because depthBoundsTestEnable is false)
	};

	depthStencilStateCreateInfo.back = depthStencilStateCreateInfo.front;	// I'm lazy :)


	/*
	 * Specify Multisample parameters.
	 *
	 */
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,  // Only one sample per pixel (effectively disables multisampling)
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};


	/*
	 * Finally, create the pipeline with all the information we defined before.
	 *
	 */
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
	    .pStages = shaderStageCreateInfo,
		.pVertexInputState = &vertexInputStateCreateInfo,
		.pInputAssemblyState = &inputAssemblyStateCreateInfo,
		.pTessellationState = nullptr,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizationStateCreateInfo,
		.pMultisampleState = &multisampleStateCreateInfo,
		.pDepthStencilState = &depthStencilStateCreateInfo,
		.pColorBlendState = &colorBlendStateCreateInfo,
		.pDynamicState = &dynamicStateCreateInfo,
		.layout = thePipelineLayout,
		.renderPass = theRenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // This and the following parameter are used
		.basePipelineIndex = -1,              //  to create derivative pipelines -- not used in this demo.
	};

	VkPipeline myPipeline;
	result = vkCreateGraphicsPipelines(theDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &myPipeline);
	assert(result == VK_SUCCESS);

	vkDestroyShaderModule(theDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(theDevice, fragmentShaderModule, nullptr);

	outPipeline = myPipeline;
	return true;
}




/**
 * Good ol' main function.
 */
int main(int argc, char* argv[])
{
	static int windowWidth = 800;
	static int windowHeight = 600;
	static const char * applicationName = "SdlVulkanDemo_02_triangle";
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
	 * Basic Vulkan initialization; we create a VkInstance, VkPhysicalDevice, VkDevice & VkQueue, and a swapchain.
	 * For more informations on this process, refer to Demo 01 and to the implementations of the various functions
	 * in directory "00_commons".
	 */
	std::vector<const char *> layersNamesToEnable;
	layersNamesToEnable.push_back("VK_LAYER_GOOGLE_threading");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_param_checker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_device_limits");
	//layersNamesToEnable.push_back("VK_LAYER_LUNARG_object_tracker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_image");
	//layersNamesToEnable.push_back("VK_LAYER_LUNARG_mem_tracker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_draw_state");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_swapchain");
	layersNamesToEnable.push_back("VK_LAYER_GOOGLE_unique_objects");

	std::vector<const char *> extensionsNamesToEnable;
	extensionsNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME); // TODO: add support for other windowing systems

	VkInstance myInstance;
	boolResult = vkdemos::createVkInstance(layersNamesToEnable, extensionsNamesToEnable, applicationName, engineName, myInstance);
	assert(boolResult);

	VkDebugReportCallbackEXT myDebugReportCallback;
	vkdemos::createDebugReportCallback(myInstance,
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT /*| VK_DEBUG_REPORT_INFORMATION_BIT_EXT*/ | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
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
	boolResult = vkdemos::createVkSwapchain(myPhysicalDevice, myDevice, mySurface, windowWidth, windowHeight, VK_NULL_HANDLE, mySwapchain, mySurfaceFormat);
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

	VkCommandBuffer myCmdBufferPresent;
	boolResult = vkdemos::allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, myCmdBufferPresent);
	assert(boolResult);

	VkFence myPresentFence;
	result = vkdemos::utils::createFence(myDevice, myPresentFence);
	assert(result == VK_SUCCESS);


	/*
	 * New initializations for this demo.
	 */
	VkPhysicalDeviceMemoryProperties myMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(myPhysicalDevice, &myMemoryProperties);

	// Create the Depth Buffer's Image and View.
	const VkFormat myDepthBufferFormat = VK_FORMAT_D16_UNORM;

	VkImage myDepthImage;
	VkImageView myDepthImageView;
	VkDeviceMemory myDepthMemory;
	boolResult = createAndAllocateImage(myDevice,
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
	boolResult = createTriangleDemoRenderPass(myDevice, mySurfaceFormat, myDepthBufferFormat, myRenderPass);
	assert(boolResult);

	// Create the Framebuffers, based on the number of swapchain images.
	std::vector<VkFramebuffer> myFramebuffersVector;
	myFramebuffersVector.reserve(mySwapchainImageViewsVector.size());

	for(const auto view : mySwapchainImageViewsVector) {
		VkFramebuffer fb;
		boolResult = createFramebuffer(myDevice, myRenderPass, {view, myDepthImageView}, windowWidth, windowHeight, fb);
		assert(boolResult);
		myFramebuffersVector.push_back(fb);
	}


	// Create a buffer to use as the vertex buffer.
	const size_t vertexBufferSize = sizeof(TriangleDemoVertex)*NUM_DEMO_VERTICES;
	VkBuffer myVertexBuffer;
	VkDeviceMemory myVertexBufferMemory;
	boolResult = createAndAllocateBuffer(myDevice,
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


	// Create the pipeline.
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr,
	};

	VkPipelineLayout myPipelineLayout;
	result = vkCreatePipelineLayout(myDevice, &pipelineLayoutCreateInfo, nullptr, &myPipelineLayout);
	assert(result == VK_SUCCESS);


	// Create Pipeline.
	VkPipeline myPipeline;
	boolResult = createTriangleDemoPipeline(myDevice, myRenderPass, myPipelineLayout, myPipeline);
	assert(boolResult);


	/*
	 * We completed the creation and allocation of all the resources we need!
	 * Now it's time to build and submit the first command buffer that will contain
	 * all the initialization commands, such as transitioning the images from
	 * VK_IMAGE_LAYOUT_UNDEFINED to something sensible.
	 */

	// We fill the initialization command buffer with... the initialization commands.
	boolResult = fillInitializationCommandBuffer(myCmdBufferInitialization, mySwapchainImagesVector, myDepthImage);
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

	std::cout << "\n---- Rendering Start ----" << std::endl;

	/*
	 * Event loop
	 */
	SDL_Event sdlEvent;
	bool quit = false;

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
			quit = !renderSingleFrame(myDevice, myQueue, mySwapchain, myCmdBufferPresent, mySwapchainImagesVector, myFramebuffersVector, myRenderPass, myPipeline, myVertexBuffer, windowWidth, windowHeight, myPresentFence);
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

	vkDestroyPipeline(myDevice, myPipeline, nullptr);
	vkDestroyPipelineLayout(myDevice, myPipelineLayout, nullptr);

	// Destroy vertex buffer and free its memory.
	vkDestroyBuffer(myDevice, myVertexBuffer, nullptr);
	vkFreeMemory(myDevice, myVertexBufferMemory, nullptr);

	// Destroy framebuffers.
	for(auto framebuffer : myFramebuffersVector)
		vkDestroyFramebuffer(myDevice, framebuffer, nullptr);

	// Destroy renderpass.
	vkDestroyRenderPass(myDevice, myRenderPass, nullptr);

	// Destroy View, Image and release memory of our depth buffer.
	vkDestroyImageView(myDevice, myDepthImageView, nullptr);
	vkDestroyImage(myDevice, myDepthImage, nullptr);
	vkFreeMemory(myDevice, myDepthMemory, nullptr);

	/*
	 * For more informations on the following commands, refer to Demo 01.
	 */
	vkDestroyFence(myDevice, myPresentFence, nullptr);
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



/**
 * Fill the specified command buffer with the initialization commands for this demo.
 *
 * The commands consist in just a bunch of CmdPipelineBarrier that transition the
 * swapchain images from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
 */
bool fillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer,
                                     const std::vector<VkImage> & theSwapchainImagesVector,
                                     const VkImage theDepthImage
                                     )
{
	VkResult result;

	/*
	 * Begin recording of the command buffer
	 */
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = (VkCommandBufferUsageFlags)0,
		.pInheritanceInfo = nullptr,
	};

	result = vkBeginCommandBuffer(theCommandBuffer, &commandBufferBeginInfo);
	assert(result == VK_SUCCESS);

	/*
	 * "The pipeline barrier specifies an execution dependency such that all work performed
	 * by the set of pipeline stages included in srcStageMask of the first set of commands
	 * completes before any work performed by the set of pipeline stages
	 * included in dstStageMask of the second set of commands begins." [Section 6.5]
	*/
	VkImageMemoryBarrier imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	};

	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;	// TODO explain those bits
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	/*
	 * Add a PipelineBarrier command for each swapchain image.
	 */
	for(const auto & image : theSwapchainImagesVector)
	{
		imageMemoryBarrier.image = image;

		// TODO run a single pipeline barrier with many VkImageMemoryBarriers at once.

		vkCmdPipelineBarrier(theCommandBuffer, srcStageMask, dstStageMask,
			0,         // dependencyFlags
			0,         // memoryBarrierCount
			nullptr,   // pMemoryBarriers
			0,         // bufferMemoryBarrierCount
			nullptr,   // pBufferMemoryBarriers
			1,         // imageMemoryBarrierCount
			&imageMemoryBarrier // pImageMemoryBarriers
		);
	}


	/*
	 * Prepare the depth buffer's image, transitioning it from VK_IMAGE_LAYOUT_UNDEFINED
	 * to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
	 */
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageMemoryBarrier.image = theDepthImage;

	vkCmdPipelineBarrier(theCommandBuffer, srcStageMask, dstStageMask,
		0,         // dependencyFlags
		0,         // memoryBarrierCount
		nullptr,   // pMemoryBarriers
		0,         // bufferMemoryBarrierCount
		nullptr,   // pBufferMemoryBarriers
		1,         // imageMemoryBarrierCount
		&imageMemoryBarrier // pImageMemoryBarriers
	);

	/*
	 * End recording of the command buffer
	 */
	result = vkEndCommandBuffer(theCommandBuffer);
	return true;
}




/**
 * Fill the specified command buffer with the present commands for this demo.
 *
 * The commands consist in
 */
bool fillRenderingCommandBuffer(const VkCommandBuffer theCommandBuffer,
                                const VkFramebuffer theCurrentFramebuffer,
                                const VkRenderPass theRenderPass,
                                const VkPipeline thePipeline,
                                const VkBuffer theVertexBuffer,
                                const int width,
                                const int height
                                )
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

	// We don't need to add a pipeline barrier to transition the swapchain's image
	// from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// because we already told the driver to do the layout change for us
	// when we created the render pass. That's a very convenient feature!

	/*
	 * Record the state setup and drawing commands.
	 */
	// Begin the renderpass (passing also the clear values for all the attachments).
	const VkClearValue clearValues[2] = {
		[0] = {.color.float32 = {0.3f, 0.5f, 0.9f, 1.0f}}, // Clear color for the color attachment at index 0
		[1] = {.depthStencil  = {1.0f, 0}},                // Clear value for the depth buffer at attachment index 1
	};

	const VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = theRenderPass,
		.framebuffer = theCurrentFramebuffer,
	    .renderArea.offset = {0, 0},
	    .renderArea.extent = {(uint32_t)width, (uint32_t)height},
		.clearValueCount = 2,
		.pClearValues = clearValues,
	};

	vkCmdBeginRenderPass(theCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the pipeline.
	vkCmdBindPipeline(theCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thePipeline);

	// Set the viewport dynamic state.
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)width,
		.height = (float)height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	vkCmdSetViewport(theCommandBuffer, 0, 1, &viewport);

	// Set the scissor dynamic state.
	VkRect2D scissor = {
		.offset.x = 0,
		.offset.y = 0,
		.extent.width = (uint32_t)width,
		.extent.height = (uint32_t)height,
	};

	vkCmdSetScissor(theCommandBuffer, 0, 1, &scissor);

	// Bind the vertex buffer.
	VkDeviceSize buffersOffsets = 0;
	vkCmdBindVertexBuffers(theCommandBuffer, VERTEX_INPUT_BINDING, 1, &theVertexBuffer, &buffersOffsets);

	// Send the draw command, that will begin all the rendering magic
	vkCmdDraw(theCommandBuffer, 3, 1, 0, 0);

	// End the render pass commands.
	vkCmdEndRenderPass(theCommandBuffer);

	// Again, we don't need to add a pipeline barrier to transition the swapchain's image
	// from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	// because we already told the driver to do it in the render pass.

	/*
	 * End recording of the command buffer
	 */
	result = vkEndCommandBuffer(theCommandBuffer);
	return true;
}



/**
 * Renders a single frame for this demo (i.e. we clear the screen).
 * Returns true on success and false on failure.
 *
 * For a more detailed description, refer to demo 01_clearscreen.
 */
bool renderSingleFrame(const VkDevice theDevice,
                       const VkQueue theQueue,
                       const VkSwapchainKHR theSwapchain,
                       const VkCommandBuffer thePresentCmdBuffer,
                       const std::vector<VkImage> & theSwapchainImagesVector,
                       const std::vector<VkFramebuffer> & theFramebuffersVector,
                       const VkRenderPass theRenderPass,
                       const VkPipeline thePipeline,
                       const VkBuffer theVertexBuffer,
                       const int width,
                       const int height,
                       const VkFence thePresentFence
                       )
{
	VkResult result;
	VkSemaphore imageAcquiredSemaphore, renderingCompletedSemaphore;

	result = vkdemos::utils::createSemaphore(theDevice, imageAcquiredSemaphore);
	assert(result == VK_SUCCESS);

	result = vkdemos::utils::createSemaphore(theDevice, renderingCompletedSemaphore);
	assert(result == VK_SUCCESS);


	/*
	 * Wait on the previous frame's fence so that we don't render frames too fast.
	 */
	vkWaitForFences(theDevice, 1, &thePresentFence, VK_TRUE, UINT64_MAX);
	vkResetFences(theDevice, 1, &thePresentFence);


	/*
	 * Acquire the index of the next available swapchain image.
	 */
	uint32_t imageIndex = UINT32_MAX;
	result = vkAcquireNextImageKHR(theDevice, theSwapchain, UINT64_MAX, imageAcquiredSemaphore, thePresentFence, &imageIndex);

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
	bool boolResult = fillRenderingCommandBuffer(thePresentCmdBuffer, theFramebuffersVector[imageIndex], theRenderPass, thePipeline, theVertexBuffer, width, height);
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
	 * Wait for the queue to complete working; see Demo 01 for a discussion about this function.
	 */
	vkQueueWaitIdle(theQueue);


	/*
	 * Cleanup
	 */
	vkDestroySemaphore(theDevice, imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(theDevice, renderingCompletedSemaphore, nullptr);
	return true;
}
