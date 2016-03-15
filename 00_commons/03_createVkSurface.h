#ifndef VKDEMOS_CREATEVKSURFACE_H
#define VKDEMOS_CREATEVKSURFACE_H

// TODO add support for other windowing systems
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include <cassert>

namespace vkdemos {

/*
 * Create a VkSurfaceKHR from the specified instance.
 *
 * This example is currently limited to XCB only.
 */
bool createVkSurfaceXCB(const VkInstance theInstance, xcb_connection_t * const xcbConnection, const xcb_window_t & xcbWindow, VkSurfaceKHR & outInstance)
{
	VkResult result;
	VkSurfaceKHR mySurface;

	/*
	 * Surface creation:
	 * this procedure depends on the Windowing system you use, but the steps to take
	 * are very similar to each other.
	 * Refer to the Window System Integration (WSI) chapter in the Vulkan Specification for more informations.
	 */
	VkXcbSurfaceCreateInfoKHR xlibSurfaceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.connection = xcbConnection,
		.window = xcbWindow
	};

	result = vkCreateXcbSurfaceKHR(theInstance, &xlibSurfaceCreateInfo, nullptr, &mySurface);
	assert(result == VK_SUCCESS);

	outInstance = mySurface;
	return true;
}

}

#endif
