#ifndef VKDEMOS_UTILS_H
#define VKDEMOS_UTILS_H

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <string>
#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>

namespace vkdemos {
namespace utils {

/**
 * Helper function that, given a VkResult, returns a string representation of its name.
 * Useful for logging 'n' stuff.
 */
std::string VkResultToString(const VkResult result)
{
	#define MAKE_CASE(resultcode) case resultcode : return std::string{ #resultcode };

	switch(result)
	{
		MAKE_CASE(VK_SUCCESS)
		MAKE_CASE(VK_NOT_READY)
		MAKE_CASE(VK_TIMEOUT)
		MAKE_CASE(VK_EVENT_SET)
		MAKE_CASE(VK_EVENT_RESET)
		MAKE_CASE(VK_INCOMPLETE)
		MAKE_CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
		MAKE_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
		MAKE_CASE(VK_ERROR_INITIALIZATION_FAILED)
		MAKE_CASE(VK_ERROR_DEVICE_LOST)
		MAKE_CASE(VK_ERROR_MEMORY_MAP_FAILED)
		MAKE_CASE(VK_ERROR_LAYER_NOT_PRESENT)
		MAKE_CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
		MAKE_CASE(VK_ERROR_FEATURE_NOT_PRESENT)
		MAKE_CASE(VK_ERROR_INCOMPATIBLE_DRIVER)
		MAKE_CASE(VK_ERROR_TOO_MANY_OBJECTS)
		MAKE_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
		MAKE_CASE(VK_ERROR_SURFACE_LOST_KHR)
		MAKE_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
		MAKE_CASE(VK_SUBOPTIMAL_KHR)
		MAKE_CASE(VK_ERROR_OUT_OF_DATE_KHR)
		MAKE_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
		MAKE_CASE(VK_ERROR_VALIDATION_FAILED_EXT)

		default:
		    return std::string{"<INVALID VKRETURN "} + std::to_string((int)result) + ">";
	}

	#undef MAKE_CASE
}



/**
 * Performs basic SDL2 window initialization.
 */
bool sdl2Initialization(const std::string & windowTitle,
                        const int windowWidth,
                        const int windowHeight,
                        SDL_Window * & outSdlWindow,
                        SDL_SysWMinfo & outSysWmInfo)
{
	SDL_Window *window;
	SDL_SysWMinfo info;

	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow(
		windowTitle.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowWidth,
		windowHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);

	SDL_VERSION(&info.version);   // initialize info structure with SDL version info

	if(SDL_GetWindowWMInfo(window, &info))
	{
		// TODO add support for other windowing systems
		if(info.subsystem != SDL_SYSWM_X11) {
			std::cout << "!!! ERROR: Only X11 is supported in this demo for now." << std::endl;
			return false;
		}
	}
	else
	{
		std::cout << "!!! ERROR: Couldn't get window information: " << SDL_GetError() << std::endl;
		return false;
	}

	outSdlWindow = window;
	outSysWmInfo = info;
	return true;
}



/**
 * Search a memory type with the required properties from a VkPhysicalDeviceMemoryProperties object.
 * @param theMemoryProperties memory properties of the Physical Device from where we would like to allocate our object.
 * @param memoryTypeBits memoryTypeBits field from the VkMemoryRequirements of the object we want to allocate.
 * @param requiredMemoryProperties additional properties we want the memory type to have.
 * @return A value >= 0 as the index in VkPhysicalDeviceMemoryProperties::memoryTypes, or -1 if a matching memory type couldn't be found.
 */
int findMemoryTypeWithProperties(const VkPhysicalDeviceMemoryProperties theMemoryProperties,
                                 const uint32_t memoryTypeBits,
                                 const VkMemoryPropertyFlags requiredMemoryProperties)
{
	uint32_t typeBits = memoryTypeBits;

	uint32_t len = std::min(theMemoryProperties.memoryTypeCount, 32u);
	for(uint32_t i = 0; i < len; i++)
	{
		if((typeBits & 1) == 1)
		{
			// Type is available, does it match user properties?
			if((theMemoryProperties.memoryTypes[i].propertyFlags & requiredMemoryProperties) == requiredMemoryProperties)
				return (int)i;
		}

		typeBits >>= 1;
	}

	return -1;
}



/**
 * Utility function to create a VkFence on a specified VkDevice.
 * @param theDevice the device used to create the fence.
 * @param outFence the created fence.
 * @return VkResult returned by vkCreateFence.
 */
VkResult createFence(const VkDevice theDevice, VkFence & outFence)
{

	VkFenceCreateInfo fenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0
	};

	return vkCreateFence(theDevice, &fenceCreateInfo, nullptr, &outFence);
}



/**
 * Utility function to create a VkSemaphore on a specified VkDevice.
 * @param theDevice the device used to create the semaphore.
 * @param outSemaphore the created semaphore.
 * @return VkResult returned by vkCreateSemaphore.
 */
VkResult createSemaphore(const VkDevice theDevice, VkSemaphore & outSemaphore)
{

	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};

	return vkCreateSemaphore(theDevice, &semaphoreCreateInfo, nullptr, &outSemaphore);
}



/**
 * Utility function to create a VkFramebuffer object from a set of VkImageViews.
 * @param theDevice the device used to create the framebuffer.
 * @param theRenderPass the renderpass the framebuffer will be used in.
 * @param theViewAttachmentsVector list of all the attachments that will compose this framebuffer.
 * @param width width of the framebuffer.
 * @param height height of the framebuffer.
 * @param outFramebuffer the resulting VkFramebuffer object.
 * @return VkResult returned by vkCreateFramebuffer.
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
 * Loads a SPIR-V shader from file in path "filename" and creates a VkShaderModule from it.
 * @param theDevice the device used to create the modules.
 * @param filename path and file name of the file containing the shader in SPIR-V format.
 * @param outShaderModule the created VkShaderModule object.
 * @return VkResult returned by vkCreateShaderModule.
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


}}	// vkdemos::utils

#endif
