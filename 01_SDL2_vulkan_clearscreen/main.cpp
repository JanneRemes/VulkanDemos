#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>


#include <iostream>
#include <vector>
#include <algorithm>
#include <assert.h>

#include <X11/Xlib-xcb.h> // for XGetXCBConnection()

//#define VK_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR

#include <vulkan/vulkan.h>

static int windowWidth = 800;
static int windowHeight = 600;
static const char * applicationName = "mySdlVulkanTest";
static const char * engineName = applicationName;



/**
 * debug callback; adapted from dbg_callback from vulkaninfo.c in the Vulkan SDK.
 */
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location,
             int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData)
{
	if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		std::cout << "--- ERROR: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::cout << "!!! WARNING: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		std::cout << "~~~ INFO: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		std::cout << "~~~ DEBUG: [" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg << std::endl;

	/*
	* false indicates that layer should not bail-out of an
	* API call that had validation failures. This may mean that the
	* app dies inside the driver due to invalid parameter(s).
	* That's what would happen without validation layers, so we'll
	* keep that behavior here.
	*/
	return false;
}



/**
 * Creates a VKInstance that has all the layer names in layersNamesToEnable
 * and all the extension names in extensionsNamesToEnable enabled.
 */
VkInstance createVkInstance(const std::vector<const char *> & layersNamesToEnable, const std::vector<const char *> & extensionsNamesToEnable)
{
	VkResult result;

	/*
	 * The VkApplicationInfo struct contains (optional) information
	 * about the application.
	 * It's passed to the VkInstanceCreateInfo struct through a pointer.
	 */
	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,  // Must be this value
		.pNext = nullptr,                       // Must be null (reserved for extensions)
		.pApplicationName = applicationName,    // Application name (UTF8, null terminated string)
		.applicationVersion = 1,                // Application version
		.pEngineName = engineName,              // Engine name (UTF8, null terminated string)
		.engineVersion = 1,                     // Engine version
		.apiVersion = VK_API_VERSION,           // Vulkan version the application expects to use;
		                                        // if = 0, this field is ignored, otherwise, if the implementation
		                                        // doesn't support the specified version, VK_ERROR_INCOMPATIBLE_DRIVER is returned.
	};


	/*
	 * The VkInstanceCreateInfo struct contains information regarding the creation of the VkInstance.
	 */
	VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // Must be this value
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
			std::cout << "     Name        : " << layer.layerName             << std::endl;
			std::cout << "     Spec.Version: " << layer.specVersion           << std::endl;
			std::cout << "     Impl.Version: " << layer.implementationVersion << std::endl;
			std::cout << "     Description : " << layer.description           << std::endl;
			std::cout << std::endl;
		}
		std::cout << "------------------------------------------------------------------\n" << std::endl;
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
			std::cout << "     Name        : " << ext.extensionName << std::endl;
			std::cout << "     Spec.Version: " << ext.specVersion   << std::endl;
			std::cout << std::endl;
		}

		return extPropertiesVector;
	};

	// Passing a null pointer as the name of the layer, we get the Vulkan implementation's (global) extensions,
	// and the extensions of the implicitly enabled layers.
	auto globalExtensionsVector = queryAndPrintExtensions(nullptr);

	for(const auto & layer : layerPropertiesVector)
		queryAndPrintExtensions(layer.layerName);

	std::cout << "------------------------------------------------------------------\n" << std::endl;


	/*
	 * Layers enable:
	 * Check if all the layer names we want to enable are present
	 * in the VkLayerProperties we collected before in layerPropertiesVector.
	 */
	for(const auto & layerName : layersNamesToEnable)
	{
		auto itr = std::find_if(layerPropertiesVector.begin(), layerPropertiesVector.end(),
			[&](const VkLayerProperties & extProp){
				return strcmp(layerName, extProp.layerName) == 0;
			}
		);

		if(itr == layerPropertiesVector.end()) {
			std::cout << "--- ERROR: Layer " << layerName << " was not found." << std::endl;
			exit(1);
		}
	}

	// We pass the pointer and size of the extension names vector to the VkInstanceCreateInfo struct
	instanceCreateInfo.enabledLayerCount = (uint32_t)layersNamesToEnable.size();
	instanceCreateInfo.ppEnabledLayerNames = layersNamesToEnable.data();


	/*
	 * Extensions enable:
	 * Check if all the extension names we want to enable are present
	 * in the VkExtensionProperties we collected before in globalExtensionsVector.
	 */
	for(const auto & extName : extensionsNamesToEnable)
	{
		auto itr = std::find_if(globalExtensionsVector.begin(), globalExtensionsVector.end(),
			[&](const VkExtensionProperties & extProp){
				return strcmp(extName, extProp.extensionName) == 0;
			}
		);

		if(itr == globalExtensionsVector.end()) {
			std::cout << "--- ERROR: extension " << extName << " was not found in the global extensions vector." << std::endl;
			exit(1);
		}
	}

	// We pass the pointer and size of the extension names vector to the VkInstanceCreateInfo struct
	instanceCreateInfo.enabledExtensionCount = (uint32_t)extensionsNamesToEnable.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensionsNamesToEnable.data();


	/*
	 * Debug Extension:
	 * Can't find documentation for this; this code is taken from vulkaninfo.c in the Vulkan SDK.
	 */
	VkDebugReportCallbackCreateInfoEXT dbg_info;
	memset(&dbg_info, 0, sizeof(dbg_info));
	dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	dbg_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
					 VK_DEBUG_REPORT_WARNING_BIT_EXT |
					 VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	dbg_info.pfnCallback = debugCallback;
	instanceCreateInfo.pNext = &dbg_info;


	/*
	 * Here the magic happens: the VkInstance gets created from
	 * the structs we filled before.
	 */
	VkInstance myInstance;
	result = vkCreateInstance(&instanceCreateInfo, nullptr, &myInstance);

	if (result == VK_ERROR_INCOMPATIBLE_DRIVER) {
		std::cout << "--- ERROR: Cannot create Vulkan instance, VK_ERROR_INCOMPATIBLE_DRIVER." << std::endl;
		exit(1);
	} else if(result != VK_SUCCESS) {
		std::cout << "--- ERROR: Cannot create Vulkan instance, <error name here>" << std::endl;
		exit(1);
	}

	std::cout << "+++ VkInstance created succesfully!\n" << std::endl;
	return myInstance;
}





/*
 * Create a VkSurfaceKHR from the specified instance.
 *
 * This example is currently limited to XCB only.
 */
VkSurfaceKHR createVkSurfaceXCB(const VkInstance & theInstance, xcb_connection_t * const xcbConnection, const xcb_window_t & xcbWindow)
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
		.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.connection = xcbConnection,
		.window = xcbWindow
	};

	result = vkCreateXcbSurfaceKHR(theInstance, &xlibSurfaceCreateInfo, nullptr, &mySurface);
	assert(result == VK_SUCCESS);

	std::cout << "+++ VkSurface created succesfully!\n" << std::endl;
	return mySurface;
}





struct DeviceAndQueueStruct{
	VkDevice device;
	VkQueue queue;
	uint32_t queueFamilyIndex;
};

DeviceAndQueueStruct createVkDeviceAndVkQueue(const VkInstance & theInstance, const VkSurfaceKHR & theSurface)
{
	VkResult result;

	/*
	 * Here we query the Vulkan Implementation for the physical devices
	 * available in the system. We then log their information to the console.
	 */
	// Query the number of physical devices awailable to the system
	uint32_t physicalDevicesCount = 0;
	result = vkEnumeratePhysicalDevices(theInstance, &physicalDevicesCount, nullptr);
    assert(result == VK_SUCCESS);

	if(physicalDevicesCount <= 0) {
		std::cout << "--- ERROR: no physical device found!" << std::endl;
		exit(1);
	}

	// Now query the pysical devices' actual data.
	std::vector<VkPhysicalDevice> physicalDevicesVector(physicalDevicesCount);
	result = vkEnumeratePhysicalDevices(theInstance, &physicalDevicesCount, physicalDevicesVector.data());
    assert(result == VK_SUCCESS);

	// For each physical device we query its properties and pretty-print them to the console.
	int deviceIndex = 0;
	for(const auto & phyDev : physicalDevicesVector)
	{
		VkPhysicalDeviceProperties phyDevProperties;
		vkGetPhysicalDeviceProperties(phyDev, &phyDevProperties);

		std::cout << "Found device: " << phyDevProperties.deviceName << " (index: " << (deviceIndex++) << ")" << std::endl;
		std::cout << "    apiVersion: " << phyDevProperties.apiVersion << std::endl;
		std::cout << " driverVersion: " << phyDevProperties.driverVersion << std::endl;
		std::cout << "      vendorID: " << phyDevProperties.vendorID << std::endl;
		std::cout << "      deviceID: " << phyDevProperties.deviceID << std::endl;
		std::cout << "    deviceType: (" << phyDevProperties.deviceType << ") ";
		switch(phyDevProperties.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:          std::cout << "OTHER";          break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: std::cout << "INTEGRATED_GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   std::cout << "DISCRETE_GPU";   break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    std::cout << "VIRTUAL_GPU";    break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:            std::cout << "CPU";            break;
			default:                                     std::cout << "UNKNOWN!!!";     break;
		}

		std::cout << std::endl;
		// VkPhysicalDeviceLimits and VkPhysicalDeviceSparseProperties not logged for simplicity.
	}

	// For simplicity, just pick the first physical device listed and proceed
	const int deviceToUseIndex = 0;  // Change this value if you are on a multi-gpu system and you want to use a different device.

	VkPhysicalDevice & phyDevice = physicalDevicesVector[deviceToUseIndex];


	/*
	 * Device Layers and Extensions:
	 * like we did for VkInstance, we need to query the implementation for the layers
	 * and the extensions a certain physical device supports.
	 * Since it's basically the same code as before (using
	 * vkEnumerateDeviceLayerProperties/vkEnumerateDeviceExtensionProperties
	 * instead of vkEnumerateInstanceLayerProperties/vkEnumerateInstanceExtensionProperties),
	 * here we just check if the device supports the VK_KHR_SWAPCHAIN_EXTENSION_NAME extension,
	 * and we assume it supports all the validation layers we ask to use.
	 *
	 * Ideally, you would be doing this for each physical device (like in the previous For cycle),
	 * as part of the procedure for choosing the physical device, but for simplicity
	 * we'll do it only once for the selected "phyDevice".
	 */
	uint32_t deviceExtensionCount = 0;
	result = vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &deviceExtensionCount, nullptr);
	assert(result == VK_SUCCESS && deviceExtensionCount > 0);

	std::vector<VkExtensionProperties> deviceExtensionVector(deviceExtensionCount);
	result = vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &deviceExtensionCount, deviceExtensionVector.data());
	assert(result == VK_SUCCESS);

	bool hasSwapchainExtension = false;
	for(const auto & extProp : deviceExtensionVector)
	{
		if(strcmp(extProp.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
			hasSwapchainExtension = true;
			break;
		}
	}

	if(!hasSwapchainExtension) {
		std::cout << "--- ERROR: chosen physical device does not support VK_KHR_SWAPCHAIN_EXTENSION_NAME!" << std::endl;
		exit(1);
	}


	/*
	 * Queue Families:
	 * we query the selected device for the number and type of Queue Families it supports,
	 * and we log to the console the properties of each.
	 *
	 * We then choose a queue family that supports graphics commands and presentation.
	 * (This demo supports, for now, only one queue that must support both graphics and present)
	 */
	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyPropertyCount, nullptr);

	if(queueFamilyPropertyCount <= 0) {
		std::cout << "--- ERROR: chosen physical device has no queue families!" << std::endl;
		exit(1);
	}

	std::vector<VkQueueFamilyProperties> queueFamilyPropertiesVector(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyPropertyCount, queueFamilyPropertiesVector.data());

	int queueFamilyIndex = 0;
	int indexOfGraphicsQueueFamily = -1;
	for(const auto & queueFamProp : queueFamilyPropertiesVector)
	{
		// Check if the queue family supports presentation
		VkBool32 doesItSupportPresent = VK_FALSE;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, (uint32_t)queueFamilyIndex, theSurface, &doesItSupportPresent);
		assert(result == VK_SUCCESS);

		std::cout << "Properties for queue family " << queueFamilyIndex << std::endl;
		std::cout << "                    queueFlags:";

		if(queueFamProp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			std::cout << " GRAPHICS";
		if(queueFamProp.queueFlags & VK_QUEUE_COMPUTE_BIT)
			std::cout << " COMPUTE";
		if(queueFamProp.queueFlags & VK_QUEUE_TRANSFER_BIT)
			std::cout << " TRANSFER";
		if(queueFamProp.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			std::cout << " SPARSE_BINDING";

		std::cout << '\n';
		std::cout << "                    queueCount: " << queueFamProp.queueCount << std::endl;
		std::cout << "            timestampValidBits: " << queueFamProp.timestampValidBits << std::endl;
		std::cout << "   minImageTransferGranularity: " << queueFamProp.minImageTransferGranularity.width
		                                        << ", " << queueFamProp.minImageTransferGranularity.height
		                                        << ", " << queueFamProp.minImageTransferGranularity.depth
		                                        << std::endl;

		std::cout << "          doesItSupportPresent: " << std::boolalpha << bool(doesItSupportPresent) << std::endl;

		// Select queue family if it supports all the requisites.
		if(bool(queueFamProp.queueFlags & VK_QUEUE_GRAPHICS_BIT) && doesItSupportPresent == VK_TRUE) {
			if(indexOfGraphicsQueueFamily < 0)
				indexOfGraphicsQueueFamily = queueFamilyIndex;
		}

		queueFamilyIndex++;
	}

	if(indexOfGraphicsQueueFamily < 0) {
		std::cout << "--- ERROR: chosen physical device has no queue families that support both graphics and present!" << std::endl;
		exit(1);
	}


	/*
	 * Queue create info:
	 * When we create the VkDevice, we also create many queues associated to it.
	 * For this reason, before we create our VkDevice, we need to specify how many queues
	 * (and with which properties) we want to have in the VkDevice.
	 *
	 * For this demo, we're going to create a single graphics queue for the device.
	 * This queue will be of the family we found before, which we already checked supports graphics.
	 */
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfoVector;

	VkDeviceQueueCreateInfo qciToFill;
	qciToFill.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qciToFill.pNext = nullptr;
	qciToFill.flags = 0;

	float queuePriority = 1.0f;
	qciToFill.queueFamilyIndex = (uint32_t)indexOfGraphicsQueueFamily; // The queue family with the graphics capability we found before.
	qciToFill.queueCount = 1;                                          // We only want one queue created for this family in this demo.
	qciToFill.pQueuePriorities = &queuePriority;                       // An array of queueCount elements specifying priorities of work
	                                                                   //  that will be submitted to each created queue. Refer to the spec for more info.
	deviceQueueCreateInfoVector.push_back(qciToFill);


	/*
	 * Physical device features:
	 * At device creation time, you can choose from a variety of fine-grained features
	 * you want to enable/disable for that particular device.
	 *
	 * For this demo we're going to use the default settings;
	 * refer to Chapter 31 (Features, Limits, and Formats) to learn more.
	 */
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(phyDevice, &physicalDeviceFeatures);


	/*
	 * Device creation:
	 * we can finally create a VkDevice, from the VkPhysicalDevice we selected before.
	 * To do that, we need to populate a VkDeviceCreationInfo struct with the relevant informations.
	 */
	VkDevice myDevice;

	std::vector<const char *> layerNamesToEnable = { "VK_LAYER_LUNARG_standard_validation" };
	std::vector<const char *> extensionNamesToEnable = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo deviceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .queueCreateInfoCount = (uint32_t)deviceQueueCreateInfoVector.size(),
	    .pQueueCreateInfos = deviceQueueCreateInfoVector.data(),
	    .enabledLayerCount = (uint32_t)layerNamesToEnable.size(),
	    .ppEnabledLayerNames = layerNamesToEnable.data(),
	    .enabledExtensionCount = (uint32_t)extensionNamesToEnable.size(),
	    .ppEnabledExtensionNames = extensionNamesToEnable.data(),
	    .pEnabledFeatures = &physicalDeviceFeatures
	};

	result = vkCreateDevice(phyDevice, &deviceCreateInfo, nullptr, &myDevice);
	assert(result == VK_SUCCESS);


	/*
	 * Since the queues are created together with the VkDevice,
	 * using the vkGetDeviceQueue we can get the created instances
	 * of the requested queues from each device.
	 *
	 * In this demo we only created a single queue.
	 */
	VkQueue myQueue;

	vkGetDeviceQueue(myDevice,                              // The device from which we want to get the queue instance
	                 (uint32_t)indexOfGraphicsQueueFamily,  // The index of the queue family were we created our queue
	                 0,                                     // The index of the created queue in the family (0 means the first one, since we created only one queue)
	                 &myQueue                               // The queue goes here.
	);

	// We're done here!
	std::cout << "\n+++ VkDevice and VkQueue created succesfully!\n" << std::endl;
	return {myDevice, myQueue, (uint32_t)indexOfGraphicsQueueFamily};
}









int main(int argc, char* argv[])
{
	/*
	 * SDL2 Initialization
	 */

	SDL_Window *window;
	SDL_SysWMinfo info;

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(
		applicationName,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		windowWidth,
		windowHeight,
		SDL_WINDOW_SHOWN
	);

	SDL_VERSION(&info.version);   // initialize info structure with SDL version info

	if(SDL_GetWindowWMInfo(window, &info))
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

		std::cout << "~~~ This program is running SDL version "
		          << (int)info.version.major << '.'
		          << (int)info.version.minor << '.'
		          << (int)info.version.patch << " on "
		          << subsystem
		          << std::endl;
	}
	else
	{
		std::cout << "--- ERROR: Couldn't get window information: " << SDL_GetError() << std::endl;
		exit(1);
	}


	/*
	 * Do all the Vulkan goodies here.
	 */

	// Vector of the layer names we want to enable on the Instance
	std::vector<const char *> layersNamesToEnable;
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_threading");       // Enable all the standard validation layers that come with the VulkanSDK.
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_param_checker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_device_limits");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_object_tracker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_image");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_mem_tracker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_draw_state");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_swapchain");
	layersNamesToEnable.push_back("VK_LAYER_GOOGLE_unique_objects");

	// Vector of the extension names we want to enable on the Instance
	std::vector<const char *> extensionsNamesToEnable;
	extensionsNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensionsNamesToEnable.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME); // TODO: add support for other windowing systems

	// Create Vulkan Objects
	VkInstance myInstance = createVkInstance(layersNamesToEnable, extensionsNamesToEnable);
	VkSurfaceKHR mySurface = createVkSurfaceXCB(myInstance, XGetXCBConnection(info.info.x11.display), static_cast<xcb_window_t>(info.info.x11.window));

	auto deviceAndQueue = createVkDeviceAndVkQueue(myInstance, mySurface);
	VkDevice myDevice = deviceAndQueue.device;
	VkQueue myQueue = deviceAndQueue.queue;



	/*
	 * Deinitialization
	 */

	// There's no function for destroying a queue; all queues of a particular
	// device are destroyed when the device is destroyed.
	vkDestroyDevice(myDevice, nullptr);
	vkDestroySurfaceKHR(myInstance, mySurface, nullptr);
	vkDestroyInstance(myInstance, nullptr);

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
