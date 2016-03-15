#ifndef VKDEMOS_UTILS_H
#define VKDEMOS_UTILS_H

#include <vulkan/vulkan.h>
#include <string>

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

}}	// vkdemos::utils

#endif
