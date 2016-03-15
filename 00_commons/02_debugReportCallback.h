#ifndef VKDEMOS_DEBUGREPORTCALLBACK_H
#define VKDEMOS_DEBUGREPORTCALLBACK_H

#include <vulkan/vulkan.h>

#include <iostream>
#include <iomanip>
#include <cassert>

namespace vkdemos {

/**
 * debug callback; adapted from dbg_callback from vulkaninfo.c in the Vulkan SDK.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location,
             int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData)
{
	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		std::cout << "!!!  ERR";
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::cout << "!!! WARN";
	else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		std::cout << "~~~ INFO";
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		std::cout << "~~~ DEBG";
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		std::cout << "~~~ PERF";
	else
		std::cout << "??? WTF?";

	std::cout << ": [" << std::setw(3) << pLayerPrefix << "]" << std::setw(3) << msgCode << ": " << pMsg << std::endl;

	/*
	* False indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return false;
}



/*
 * Create a VkDebugReportCallbackEXT on the specified Instance.
 *
 * This way we'll get messages from the driver or the validation layers.
 */
bool createDebugReportCallback(const VkInstance theInstance,
                               const VkDebugReportFlagsEXT theFlags,
                               const PFN_vkDebugReportCallbackEXT theCallback,
                               VkDebugReportCallbackEXT & outDebugReportCallback)
{
	// Since this is an extension, we need to get the pointer to vkCreateDebugReportCallbackEXT at runtime
	auto pfn_vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(theInstance, "vkCreateDebugReportCallbackEXT");

	// Fill a VkDebugReportCallbackCreateInfoEXT struct with appropriate information
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
		.pNext = 0,
		.flags = theFlags,
		.pfnCallback = theCallback,
		.pUserData = nullptr,
	};

	// Call the function pointer we got before to create the VkDebugReportCallback:
	VkDebugReportCallbackEXT myDebugReportCallback;
	VkResult result = pfn_vkCreateDebugReportCallbackEXT(theInstance, &debugReportCallbackCreateInfo, nullptr, &myDebugReportCallback);
	assert(result == VK_SUCCESS);

	outDebugReportCallback = myDebugReportCallback;
	return true;
}



/*
 * Utility to destroy a VkDebugReportCallbackEXT
 */
void destroyDebugReportCallback(const VkInstance theInstance, const VkDebugReportCallbackEXT theDebugReportCallback)
{
	// Get the function pointer and call the function.
	auto pfn_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(theInstance, "vkDestroyDebugReportCallbackEXT");
	pfn_vkDestroyDebugReportCallbackEXT(theInstance, theDebugReportCallback, nullptr);
}

}

#endif
