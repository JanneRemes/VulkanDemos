#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>


#include <iostream>
#include <vector>
#include <assert.h>

#include <vulkan/vulkan.h>

int windowWidth = 800;
int windowHeight = 600;



void createVkInstance()
{
	VkInstance myInstance;
	VkResult result;

	/*
	 * The VkApplicationInfo struct contains (optional) information
	 * about the application.
	 * It's passed to the VkInstanceCreateInfo struct through a pointer.
	 */
	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, // Must be this value
		.pNext = nullptr,                        // Must be null (reserved for extensions)
		.pApplicationName = "mySdlVulkanTest",   // Application name (UTF8, null terminated string)
		.applicationVersion = 1,                 // Application version
		.pEngineName = "mySdlVulkanTest",        // Engine name (UTF8, null terminated string)
		.engineVersion = 1,                      // Engine version
		.apiVersion = VK_API_VERSION,            // Vulkan version the application expects to use;
		                                         // if = 0, this field is ignored, otherwise, if the implementation
		                                         // doesn't support the specified version, VK_ERROR_INCOMPATIBLE_DRIVER is returned.
	};

	/*
	 * The VkInstanceCreateInfo struct contains information regarding the creation of the VkInstance.
	 */
	VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // Must be this value
		.pNext = nullptr,                       // Must be null (reserved for extensions)
		.flags = 0,                             // Must be 0 (reserved for future use)
		.pApplicationInfo = &applicationInfo,   // Pointer to a VkApplicationInfo struct (or can be null)
		.enabledLayerCount = 0,                 // Number of global layers to enable
		.ppEnabledLayerNames = nullptr,         // Pointer to array of #enabledLayerCount strings containing the names of the layers to enable
		.enabledExtensionCount = 0,             // Number of global extensions to enable
		.ppEnabledExtensionNames = nullptr,     // Pointer to array of #enabledExtensionCount strings containing the names of the extensions to enable
	};


	/*
	 * Layers:
	 * We list all the layers the current implementation supports,
	 * and we log them to the console.
	 */
	std::vector<VkLayerProperties> layerPropertiesVector;

	{
		uint32_t supportedLayersCount = 0;

		// Calling vkEnumerateInstanceLayerProperties with a null pProperties will yeld
		// the number of supported layer in pPropertyCount ("supportedLayersCount" here).
		result = vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr);
		assert(result == VK_SUCCESS);

		// Allocate a vector to keep all the VkLayerProperties structs.
		layerPropertiesVector.resize(supportedLayersCount);

		// Get the LayerProperties.
		result = vkEnumerateInstanceLayerProperties(&supportedLayersCount, layerPropertiesVector.data());
		assert(result == VK_SUCCESS);

		// Log all the layers on the console.
		std::cout << "Found " << layerPropertiesVector.size() << " layers:" << std::endl;

		for(const auto & layer : layerPropertiesVector)
		{
			std::cout << std::endl;
			std::cout << "  Name        : " << layer.layerName             << std::endl;
			std::cout << "  Spec.Version: " << layer.specVersion           << std::endl;
			std::cout << "  Impl.Version: " << layer.implementationVersion << std::endl;
			std::cout << "  Description : " << layer.description           << std::endl;
		}
		std::cout << "------------------------------------------------------------------\n" << std::endl;

		/*
		 * Here we can populate a vector of layer names we want to enable,
		 * and pass it through the ppEnabledLayerNames pointer inside VkInstanceCreateInfo.
		 */
	}


	/*
	 * Extensions:
	 * Each layer has its own set of extensions; for each layer, we query the extensions it provides
	 * and we log them to the console.
	 * Note that the Vulkan implementation could provide extensions, as other implicitly enabled layers;
	 * we query those extensions too.
	 */

	// Helper lambda to query extensions for a certain layer name and print them to console
	const auto queryAndPrintExtensions = [&](const char * layerName)
	{
		uint32_t propertyCount = 0;

		// Query the number of extensions
		result = vkEnumerateInstanceExtensionProperties(layerName, &propertyCount, nullptr);
		assert(result == VK_SUCCESS);

		// Allocate a vector to store the extension properties
		std::vector<VkExtensionProperties> extPropertiesVector(propertyCount);

		// Query the actual properties
		result = vkEnumerateInstanceExtensionProperties(layerName, &propertyCount, extPropertiesVector.data());
		assert(result == VK_SUCCESS);

		// Log them to console
		std::cout << "Found " << extPropertiesVector.size() << " extensions for layer " << (layerName==nullptr ? "(null)" : layerName) << std::endl;

		for(const auto & ext : extPropertiesVector) {
			std::cout << std::endl;
			std::cout << "  Name        : " << ext.extensionName << std::endl;
			std::cout << "  Spec.Version: " << ext.specVersion   << std::endl;
		}
		std::cout << "------------------------------------------------------------------\n" << std::endl;
	};

	// Passing a null pointer as the name of the layer, we get the Vulkan implementation's extensions,
	// or the extensions of the implicitly enabled layers.
	queryAndPrintExtensions(nullptr);

	for(const auto & layer : layerPropertiesVector)
		queryAndPrintExtensions(layer.layerName);


/*





	// Global Extensions to enable
	static const char *known_extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
		#ifdef VK_USE_PLATFORM_ANDROID_KHR
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
		#endif
		#ifdef VK_USE_PLATFORM_MIR_KHR
			VK_KHR_MIR_SURFACE_EXTENSION_NAME,
		#endif
		#ifdef VK_USE_PLATFORM_WAYLAND_KHR
			VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
		#endif
		#ifdef VK_USE_PLATFORM_WIN32_KHR
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		#endif
		#ifdef VK_USE_PLATFORM_XCB_KHR
			VK_KHR_XCB_SURFACE_EXTENSION_NAME,
		#endif
		#ifdef VK_USE_PLATFORM_XLIB_KHR
			VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
		#endif
	};

	uint32_t global_extension_count = 0;

	inst.global_extension_count = 0;
	app_get_global_layer_extensions(NULL, &inst.global_extension_count,
									&inst.global_extensions);

	for (uint32_t i = 0; i < ARRAY_SIZE(known_extensions); i++) {
		VkBool32 extension_found = 0;
		for (uint32_t j = 0; j < inst.global_extension_count; j++) {
			VkExtensionProperties *extension_prop = &inst.global_extensions[j];
			if (!strcmp(known_extensions[i], extension_prop->extensionName)) {

				extension_found = 1;
				global_extension_count++;
			}
		}
		if (!extension_found) {
			printf("Cannot find extension: %s\n", known_extensions[i]);
			ERR_EXIT(VK_ERROR_EXTENSION_NOT_PRESENT);
		}
	}

	inst_info.enabledExtensionCount = global_extension_count;
	inst_info.ppEnabledExtensionNames = (const char *const *)known_extensions;




	VkDebugReportCallbackCreateInfoEXT dbg_info;
	memset(&dbg_info, 0, sizeof(dbg_info));
	dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	dbg_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
					 VK_DEBUG_REPORT_WARNING_BIT_EXT |
					 VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	dbg_info.pfnCallback = dbg_callback;
	inst_info.pNext = &dbg_info;


*/


	// Here the magic happens
	result = vkCreateInstance(&instanceCreateInfo, nullptr, &myInstance);

	// Error checking
	if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
		std::cout << "ERROR: Cannot create Vulkan instance, VK_ERROR_INCOMPATIBLE_DRIVER." << std::endl;
		exit(1);
	} else if(result != VK_SUCCESS) {
		std::cout << "ERROR: Cannot create Vulkan instance, <error name here>" << std::endl;
		exit(1);
	}




	// FIXME Keep things clean for now
	vkDestroyInstance(myInstance, nullptr);

}















int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);   // Initialize SDL2

	SDL_Window *window;        // Declare a pointer to an SDL_Window
	SDL_SysWMinfo info;

	// Create an application window with the following settings:
	window = SDL_CreateWindow(
		"An SDL2 window",         // const char* title
		SDL_WINDOWPOS_UNDEFINED,  // int x: initial x position
		SDL_WINDOWPOS_UNDEFINED,  // int y: initial y position
		windowWidth,              // int w: width, in pixels
		windowHeight,             // int h: height, in pixels
		SDL_WINDOW_SHOWN          // Uint32 flags: window options, see docs
	);


	SDL_VERSION(&info.version);   // initialize info structure with SDL version info

	if(SDL_GetWindowWMInfo(window,&info))
	{
		const char *subsystem = "an unknown system!";
		switch(info.subsystem)
		{
			case SDL_SYSWM_UNKNOWN:   break;
			case SDL_SYSWM_WINDOWS:   subsystem = "Microsoft Windows(TM)";  break;
			case SDL_SYSWM_X11:       subsystem = "X Window System";        break;
		#if SDL_VERSION_ATLEAST(2, 0, 3)
			case SDL_SYSWM_WINRT:     subsystem = "WinRT";                  break;
		#endif
			case SDL_SYSWM_DIRECTFB:  subsystem = "DirectFB";               break;
			case SDL_SYSWM_COCOA:     subsystem = "Apple OS X";             break;
			case SDL_SYSWM_UIKIT:     subsystem = "UIKit";                  break;
		#if SDL_VERSION_ATLEAST(2, 0, 2)
			case SDL_SYSWM_WAYLAND:   subsystem = "Wayland";                break;
			case SDL_SYSWM_MIR:       subsystem = "Mir";                    break;
		#endif
		#if SDL_VERSION_ATLEAST(2, 0, 4)
			case SDL_SYSWM_ANDROID:   subsystem = "Android";                break;
		#endif
		}

		SDL_Log("This program is running SDL version %d.%d.%d on %s\n",
			(int)info.version.major,
			(int)info.version.minor,
			(int)info.version.patch,
			subsystem);
	}
	else
	{
		// call failed
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Couldn't get window information: %s\n", SDL_GetError());
	}





	createVkInstance();




	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;


}

