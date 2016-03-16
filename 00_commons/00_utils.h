#ifndef VKDEMOS_UTILS_H
#define VKDEMOS_UTILS_H

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <string>
#include <iostream>

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


}}	// vkdemos::utils

#endif
