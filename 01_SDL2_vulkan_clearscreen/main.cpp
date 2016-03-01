#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>


#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cassert>

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
		std::cout << "---   ERR";
	else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		std::cout << "!!!  WARN";
	else if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
		std::cout << "~~~  INFO";
	else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		std::cout << "~~~ DEBUG";
	else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		std::cout << "~~~ PERFW";
	else
		std::cout << "~~~ ?WTF?";

	std::cout << ": [" << std::setw(3) << pLayerPrefix << "]" << std::setw(3) << msgCode << ": " << pMsg << std::endl;

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
bool createVkInstance(const std::vector<const char *> & layersNamesToEnable, const std::vector<const char *> & extensionsNamesToEnable, VkInstance & outInstance)
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
			return false;
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
			return false;
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
		return false;
	} else if(result != VK_SUCCESS) {
		std::cout << "--- ERROR: Cannot create Vulkan instance, <error name here>" << std::endl;
		return false;
	}

	std::cout << "+++ VkInstance created succesfully!\n" << std::endl;
	outInstance = myInstance;
	return true;
}




/*
 * Create a Debug Report callback on the specified Instance.
 *
 * This way we'll get messages from the driver or the validation layers.
 */
bool createDebugReportCallback(const VkInstance theInstance, const VkDebugReportFlagsEXT theFlags, const PFN_vkDebugReportCallbackEXT theCallback, VkDebugReportCallbackEXT & outDebugReportCallback)
{
	//Since this is an extension, we need to get the pointer to vkCreateDebugReportCallbackEXT at runtime
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
 * Utility to destroy a DebugReportCallback
 */
void destroyDebugReportCallback(const VkInstance theInstance, const VkDebugReportCallbackEXT theDebugReportCallback)
{
	// Get the function pointer
	auto pfn_vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(theInstance, "vkDestroyDebugReportCallbackEXT");
	pfn_vkDestroyDebugReportCallbackEXT(theInstance, theDebugReportCallback, nullptr);
}




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

	std::cout << "+++ VkSurface created succesfully!\n" << std::endl;
	outInstance = mySurface;
	return true;
}




/**
 * Query the instance for all the physical devices, and return one of them.
 * (in this demo, for simplicity, we take the first phydev we find;
 * ideally, we would query the capabilities of each phydev and choose the
 * one best suited for our needs).
 */
bool chooseVkPhysicalDevice(const VkInstance theInstance, VkPhysicalDevice & outPhysicalDevice)
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
		return false;
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

		std::cout << "Found physical device: " << phyDevProperties.deviceName << " (index: " << (deviceIndex++) << ")" << std::endl;
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

	std::cout << "\n+++ Index of physical device choosen: " << deviceToUseIndex << '\n' << std::endl;

	outPhysicalDevice = physicalDevicesVector[deviceToUseIndex];
	return true;
}




/**
 * Creates a VkDevice and its associated VkQueue.
 */
bool createVkDeviceAndVkQueue(const VkPhysicalDevice thePhysicalDevice, const VkSurfaceKHR theSurface, VkDevice & outDevice, VkQueue & outQueue, uint32_t & outQueueFamilyIndex)
{
	VkResult result;

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
	 * Ideally, you would be doing this for each physical device
	 * as part of the procedure for choosing the physical device, but for simplicity
	 * we'll do it only once for the selected "phyDevice".
	 */
	uint32_t deviceExtensionCount = 0;
	result = vkEnumerateDeviceExtensionProperties(thePhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
	assert(result == VK_SUCCESS && deviceExtensionCount > 0);

	std::vector<VkExtensionProperties> deviceExtensionVector(deviceExtensionCount);
	result = vkEnumerateDeviceExtensionProperties(thePhysicalDevice, nullptr, &deviceExtensionCount, deviceExtensionVector.data());
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
		return false;
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
	vkGetPhysicalDeviceQueueFamilyProperties(thePhysicalDevice, &queueFamilyPropertyCount, nullptr);

	if(queueFamilyPropertyCount <= 0) {
		std::cout << "--- ERROR: chosen physical device has no queue families!" << std::endl;
		return false;
	}

	std::vector<VkQueueFamilyProperties> queueFamilyPropertiesVector(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(thePhysicalDevice, &queueFamilyPropertyCount, queueFamilyPropertiesVector.data());

	int queueFamilyIndex = 0;
	int indexOfGraphicsQueueFamily = -1;
	for(const auto & queueFamProp : queueFamilyPropertiesVector)
	{
		// Check if the queue family supports presentation
		VkBool32 doesItSupportPresent = VK_FALSE;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(thePhysicalDevice, (uint32_t)queueFamilyIndex, theSurface, &doesItSupportPresent);
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
		return false;
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
	vkGetPhysicalDeviceFeatures(thePhysicalDevice, &physicalDeviceFeatures);


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

	result = vkCreateDevice(thePhysicalDevice, &deviceCreateInfo, nullptr, &myDevice);
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
	outDevice = myDevice;
	outQueue = myQueue;
	outQueueFamilyIndex = (uint32_t)indexOfGraphicsQueueFamily;
	return true;
}




/**
 * Create a VkSwapchain from the VkSurface provided.
 * Parameter "theOldSwapChain" is used if we are recreating a swapchain (for example if we resized the window);
 * if we are creating the swapchain for the first time, this parameter must be VK_NULL_HANDLE.
 */
bool createVkSwapchain(const VkPhysicalDevice thePhysicalDevice,
                       const VkDevice theDevice,
                       const VkSurfaceKHR theSurface,
                       VkSwapchainKHR theOldSwapChain,
                       VkSwapchainKHR & outSwapchain,
                       VkFormat & outSurfaceFormat
                       )
{
	VkResult result;

	/*
	 * Get the list of VkFormat's that are supported.
	 * TODO: document
	 */
	uint32_t surfaceFormatsCount;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(thePhysicalDevice, theSurface, &surfaceFormatsCount, nullptr);
	assert(result == VK_SUCCESS);

	std::vector<VkSurfaceFormatKHR> surfaceFormatsVector(surfaceFormatsCount);
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(thePhysicalDevice, theSurface, &surfaceFormatsCount, surfaceFormatsVector.data());
	assert(result == VK_SUCCESS);

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format. Otherwise, at least one
	// supported format will be returned.
	VkFormat surfaceFormat;

	if (surfaceFormatsCount == 1 && surfaceFormatsVector[0].format == VK_FORMAT_UNDEFINED) {
		surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
	} else {
		assert(surfaceFormatsCount >= 1);
		surfaceFormat = surfaceFormatsVector[0].format;
	}

	VkColorSpaceKHR surfaceColorSpace = surfaceFormatsVector[0].colorSpace;


	/*
	 * Get physical device surface capabilities
	 * TODO: document
	 */
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(thePhysicalDevice, theSurface, &surfaceCapabilities);
    assert(result == VK_SUCCESS);


	/*
	 * Get Physical device surface present modes.
	 * A present mode is how the Device will synchronize itself with the video screen when
	 * has rendered frames available to display.
	 */
	uint32_t presentModeCount = 0;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(thePhysicalDevice, theSurface, &presentModeCount, nullptr);
	assert(result == VK_SUCCESS && presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModesVector(presentModeCount);
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(thePhysicalDevice, theSurface, &presentModeCount, presentModesVector.data());
	assert(result == VK_SUCCESS);

	for(const auto presMode : presentModesVector)
	{
		std::cout << "--- Supported present mode: ";
		switch(presMode) {
			case VK_PRESENT_MODE_IMMEDIATE_KHR:    std::cout << "VK_PRESENT_MODE_IMMEDIATE_KHR";    break;
			case VK_PRESENT_MODE_MAILBOX_KHR:      std::cout << "VK_PRESENT_MODE_MAILBOX_KHR";      break;
			case VK_PRESENT_MODE_FIFO_KHR:         std::cout << "VK_PRESENT_MODE_FIFO_KHR";         break;
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR: std::cout << "VK_PRESENT_MODE_FIFO_RELAXED_KHR"; break;
			default: std::cout << "???"; break;
		}
		std::cout << " (" << presMode << ')' << std::endl;
	}


	// Get the swapchain extent, and check if it's already filled or if we must manually set width and height.
	VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;

	if (swapchainExtent.width == (uint32_t)(-1)) // width and height are either both -1, or both not -1.
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = (uint32_t)windowWidth;
		swapchainExtent.height = (uint32_t)windowHeight;
	}
	/*else {	// TODO investigate if it's actually a good idea (since we are using SDL, the surface SHOULD have a valid dimension, wouldn't it?)
		// If the surface size is defined, the swap chain size must match
		demo->width = surfaceCapabilities.currentExtent.width;
		demo->height = surfaceCapabilities.currentExtent.height;
	}*/


	// Determine the number of VkImage's to use in the swap chain (we desire to
	// own only 1 image at a time, besides the images being displayed and
	// queued for display):
	uint32_t desiredNumberOfSwapchainImages = surfaceCapabilities.minImageCount + 1;

	if (surfaceCapabilities.maxImageCount > 0) {
		// Limit the number of images, if maxImageCount is defined.
		desiredNumberOfSwapchainImages = std::min(desiredNumberOfSwapchainImages, surfaceCapabilities.maxImageCount);
	}

	// TODO: document this part
	VkSurfaceTransformFlagBitsKHR preTransform;

	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	else
		preTransform = surfaceCapabilities.currentTransform;


	/*
	 * Create the swapchain
	 * TODO document all the fields
	 */
	const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
	    .flags = 0,
		.surface = theSurface,
		.minImageCount = desiredNumberOfSwapchainImages,
		.imageFormat = surfaceFormat,
		.imageColorSpace = surfaceColorSpace,
		.imageExtent = swapchainExtent,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = preTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,	// TODO understand this parameter
		.oldSwapchain = theOldSwapChain,  // if we are recreating a swapchain, we pass the old one here.
		.clipped = true,
	};

	VkSwapchainKHR mySwapchain;
	result = vkCreateSwapchainKHR(theDevice, &swapchainCreateInfo, nullptr, &mySwapchain);
	assert(result == VK_SUCCESS);

	std::cout << "+++ VkSwapchainKHR created succesfully!\n";
	outSwapchain = mySwapchain;
	outSurfaceFormat = surfaceFormat;

	// Destroy the old swapchain, if there was one.
	if(theOldSwapChain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(theDevice, theOldSwapChain, nullptr);
		std::cout << "+++     ... and old VkSwapchainKHR destroyed succesfully!\n";
	}
	std::cout << std::endl;

	return true;
}




/**
 * Get the VkImages out of the specified swapchain, and create for each of them an
 * associated VkImageView.
 */
bool getSwapchainImagesAndViews(const VkDevice theDevice,
                                const VkSwapchainKHR theSwapchain,
                                const VkFormat & theSurfaceFormat,
                                std::vector<VkImage> & outSwapchainImagesVector,
                                std::vector<VkImageView> & outSwapchainImageViewsVector
                                )
{
	VkResult result;

	/*
	 * Get swapchain images
	 */
	uint32_t swapchainImageCount;
	result = vkGetSwapchainImagesKHR(theDevice, theSwapchain, &swapchainImageCount, nullptr);
	assert(result == VK_SUCCESS);

	std::vector<VkImage> swapchainImagesVector(swapchainImageCount);
	result = vkGetSwapchainImagesKHR(theDevice, theSwapchain, &swapchainImageCount, swapchainImagesVector.data());
	assert(result == VK_SUCCESS);

	std::vector<VkImageView> swapchainImageViewsVector(swapchainImageCount);

	/*
	 * Create the view for each image
	 */
	for(auto i = 0u; i < swapchainImageCount; i++)
	{
		// TODO document every field
		VkImageViewCreateInfo imageViewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.format = theSurfaceFormat,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
		    },
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
		    .image = swapchainImagesVector[i],
		};

		result = vkCreateImageView(theDevice, &imageViewCreateInfo, nullptr, &swapchainImageViewsVector[i]);
		assert(result == VK_SUCCESS);
	}

	std::cout << "+++ Created " << swapchainImageCount << " swapchain images and views."<< std::endl;
	outSwapchainImagesVector = std::move(swapchainImagesVector);
	outSwapchainImageViewsVector = std::move(swapchainImageViewsVector);
	return true;
}




/**
 * Create a VkCommandPool, from which all the VkCommandBuffer will be allocated.
 */
bool createCommandPool(const VkDevice theDevice,
                       const uint32_t theQueueFamilyIndex,
                       const VkCommandPoolCreateFlagBits createFlagBits,
                       VkCommandPool & outCommandPool
                       )
{
	VkResult result;

	const VkCommandPoolCreateInfo commandPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = createFlagBits,
	    .queueFamilyIndex = theQueueFamilyIndex,
	};

	VkCommandPool myCommandPool;
	result = vkCreateCommandPool(theDevice, &commandPoolCreateInfo, nullptr, &myCommandPool);
	assert(result == VK_SUCCESS);

	outCommandPool = myCommandPool;
	return true;
}




/**
 * Allocate a VkCommandBuffer from a VkCommandPool.
 */
bool allocateCommandBuffer(const VkDevice theDevice,
                           const VkCommandPool theCommandPool,
                           const VkCommandBufferLevel theCommandBufferLevel,
                           VkCommandBuffer & outCommandBuffer
                           )
{
	VkResult result;

	const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = theCommandPool,
	    .level = theCommandBufferLevel,
	    .commandBufferCount = 1
	};

	VkCommandBuffer myCommandBuffer;
	result = vkAllocateCommandBuffers(theDevice, &commandBufferAllocateInfo, &myCommandBuffer);
	assert(result == VK_SUCCESS);

	outCommandBuffer = myCommandBuffer;
	return true;
}




/**
 * Fill the specified command buffer with the initialization commands for this demo.
 *
 * The commands consist in just a bunch of CmdPipelineBarrier that transition the
 * swapchain images from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
 */
bool fillInitializationCommandBuffer(const VkCommandBuffer theCommandBuffer, const std::vector<VkImage> & theSwapchainImagesVector)
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
bool fillPresentCommandBuffer(const VkCommandBuffer theCommandBuffer, const VkImage theCurrentSwapchainImage, const float clearColorR, const float clearColorG, const float clearColorB)
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


	// Pre-fill a VkImageMemoryBarrier structure that we'll use later
	VkImageMemoryBarrier imageMemoryBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
	    .image = theCurrentSwapchainImage,
	};

	/*
	 * Transition the swapchain image from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	 * to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	 */
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,

	vkCmdPipelineBarrier(theCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
	);

	/*
	 * Add the CmdClearColorImage that will fill the swapchain image with the specified color.
	 * For this command to work, the destination image must be in either VK_IMAGE_LAYOUT_GENERAL
	 * or VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
	 */
	VkClearColorValue clearColorValue;
	VkImageSubresourceRange imageSubresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	clearColorValue.float32[0] = clearColorR;	// FIXME it assumes the image is a floating point one. Is it always true?
	clearColorValue.float32[1] = clearColorG;
	clearColorValue.float32[2] = clearColorB;
	clearColorValue.float32[3] = 1.0f;	// alpha
	vkCmdClearColorImage(theCommandBuffer, theCurrentSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &imageSubresourceRange);


	/*
	 * Transition the swapchain image from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	 * to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	 */
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,

	vkCmdPipelineBarrier(theCommandBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier
	);


	/*
	 * End recording of the command buffer
	 */
	result = vkEndCommandBuffer(theCommandBuffer);
	return true;
}




/**
 * Render a single frame for this demo (i.e. we clear the screen).
 * Returns true on success and false on failure.
 */
bool renderSingleFrame(const VkDevice theDevice,
                       const VkQueue theQueue,
                       const VkSwapchainKHR theSwapchain,
                       const VkCommandBuffer thePresentCmdBuffer,
                       const std::vector<VkImage> & theSwapchainImagesVector,
                       const VkFence thePresentFence)
{
	VkResult result;
	VkSemaphore imageAcquiredSemaphore, renderingCompletedSemaphore;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};

	// We create a semaphore that will be signalled when a swapchain image is ready to use,
	// and that will be waited upon by the queue before starting all the rendering/present commands.
	result = vkCreateSemaphore(theDevice, &semaphoreCreateInfo, nullptr, &imageAcquiredSemaphore);
	assert(result == VK_SUCCESS);

	// We create another semaphore that will be signalled when the queue has terminated the rendering commands,
	// and that will be waited upon by the actual present operation.
	result = vkCreateSemaphore(theDevice, &semaphoreCreateInfo, nullptr, &renderingCompletedSemaphore);
	assert(result == VK_SUCCESS);

	/*
	 * We wait on the fence so that we don't render frames too fast.
	 * FIXME it doesn't seem to work (on linux/nvidia 355 at least).
	 */
	vkWaitForFences(theDevice, 1, &thePresentFence, VK_TRUE, UINT64_MAX);
	vkResetFences(theDevice, 1, &thePresentFence);

	/*
	 * We acquire the index of the next available swapchain image.
	 */
	uint32_t imageIndex = UINT32_MAX;
	result = vkAcquireNextImageKHR(theDevice, theSwapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE/*thePresentFence*/, &imageIndex);

	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		// The swapchain is out of date (e.g. the window was resized) and must be recreated.
		// In this demo we just "gracefully crash".
		std::cout << "--- ERROR: Demo doesn't yet support out-of-date swapchains." << std::endl;
		// TODO tear down and recreate the swapchain and all its images from scratch.
		return false;
	} else if(result == VK_SUBOPTIMAL_KHR) {
		// The swapchain is not as optimal as it could be, but the platform's
		// presentation engine will still present the image correctly.
		std::cout << "~~~ Swapchain is suboptimal." << std::endl;
	} else {
		assert(result == VK_SUCCESS);
	}

	/*
	 * We fill the present command buffer with... the present commands.
	 */
	bool boolResult = fillPresentCommandBuffer(thePresentCmdBuffer, theSwapchainImagesVector[imageIndex], 1.0f, 0.2f, 0.2f);
	assert(boolResult);

	/*
	 * We submit the present command buffer to the queue (this is the fun part!)
	 *
	 * In the VkSubmitInfo we specify imageAcquiredSemaphore as a wait semaphore;
	 * this way, the submitted command buffers won't start executing before the
	 * swapchain image is ready.
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

	result = vkQueueSubmit(theQueue, 1, &submitInfo, /*VK_NULL_HANDLE*/thePresentFence);
	assert(result == VK_SUCCESS);

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
		std::cout << "--- ERROR: Demo doesn't yet support out-of-date swapchains." << std::endl;
		return false;
	} else if(result != VK_SUBOPTIMAL_KHR) {
		assert(result == VK_SUCCESS);
	}

	// We wait until all the operations in the queue have terminated.
	// In a "real" application, you'll just start rendering another frame,
	// or run some other CPU code.
	result = vkQueueWaitIdle(theQueue);
	assert(result == VK_SUCCESS);

	vkDestroySemaphore(theDevice, imageAcquiredSemaphore, nullptr);
	vkDestroySemaphore(theDevice, renderingCompletedSemaphore, nullptr);
	return true;
}




/**
 * The good ol' main function.
 */
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
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
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
	bool boolResult;
	VkResult result;

	// Vector of the layer names we want to enable on the Instance
	std::vector<const char *> layersNamesToEnable;
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_threading");       // Enable all the standard validation layers that come with the VulkanSDK.
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_param_checker");
	layersNamesToEnable.push_back("VK_LAYER_LUNARG_device_limits");
	//layersNamesToEnable.push_back("VK_LAYER_LUNARG_object_tracker");
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

	// Create a VkInstance
	VkInstance myInstance;
	boolResult = createVkInstance(layersNamesToEnable, extensionsNamesToEnable, myInstance);
	assert(boolResult);

	VkDebugReportCallbackEXT myDebugReportCallback;
	createDebugReportCallback(myInstance,
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT,
		debugCallback,
		myDebugReportCallback
	);

	// Choose a physical device from the instance
	VkPhysicalDevice myPhysicalDevice;
	boolResult = chooseVkPhysicalDevice(myInstance, myPhysicalDevice);
	assert(boolResult);

	// Create a VkSurfaceKHR
	// TODO add support for other windowing systems
	VkSurfaceKHR mySurface;
	boolResult = createVkSurfaceXCB(myInstance, XGetXCBConnection(info.info.x11.display), static_cast<xcb_window_t>(info.info.x11.window), mySurface);
	assert(boolResult);

	// Create a VkDevice and its VkQueue
	VkDevice myDevice;
	VkQueue myQueue;
	uint32_t myQueueFamilyIndex;
	boolResult = createVkDeviceAndVkQueue(myPhysicalDevice, mySurface, myDevice, myQueue, myQueueFamilyIndex);
	assert(boolResult);

	// Create a VkSwapchainKHR
	VkSwapchainKHR mySwapchain;
	VkFormat mySurfaceFormat;
	boolResult = createVkSwapchain(myPhysicalDevice, myDevice, mySurface, VK_NULL_HANDLE, mySwapchain, mySurfaceFormat);
	assert(boolResult);

	// Create the swapchain images and related views.
	// (the views are not actually used in this demo, but since they'll be needed in the future
	// to create the framebuffers, we'll create them anyway to show how to do it).
	std::vector<VkImage> mySwapchainImagesVector;
	std::vector<VkImageView> mySwapchainImageViewsVector;
	boolResult = getSwapchainImagesAndViews(myDevice, mySwapchain, mySurfaceFormat, mySwapchainImagesVector, mySwapchainImageViewsVector);
	assert(boolResult);

	// Create a command pool, so that we can later allocate the command buffers.
	VkCommandPool myCommandPool;
	boolResult = createCommandPool(myDevice, myQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, myCommandPool);
	assert(boolResult);


	// Allocate a command buffer that will hold our initialization commands.
	VkCommandBuffer myCmdBufferInitialization;
	boolResult = allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, myCmdBufferInitialization);
	assert(boolResult);

	// Allocate a command buffer that will hold our clear screen and present commands.
	VkCommandBuffer myCmdBufferPresent;
	boolResult = allocateCommandBuffer(myDevice, myCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, myCmdBufferPresent);
	assert(boolResult);

	// We create a Fence object: this object will be used to synchronize
	// the CPU thread with the GPU presentation rate, so that
	// we don't do useless work on the cpu if the presentation rate
	// is slower that the rendering rate.
	// Note: in a "real" application, you would have multiple Fences
	// (for example 2 or 3), so that the CPU doesn't wait on every frame
	// but there's a bit of buffering going on.
	VkFence myPresentFence;
	VkFenceCreateInfo fenceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0
	};
	result = vkCreateFence(myDevice, &fenceCreateInfo, nullptr, &myPresentFence);
	assert(result == VK_SUCCESS);

	/*
	 * We completed the creation and allocation of all the resources we need!
	 * Now it's time to build and submit the first command buffer that will contain
	 * all the initialization commands, such as transitioning the images from
	 * VK_IMAGE_LAYOUT_UNDEFINED to something sensible.
	 */

	// We fill the initialization command buffer with... the initialization commands.
	boolResult = fillInitializationCommandBuffer(myCmdBufferInitialization, mySwapchainImagesVector);
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
	// Note: in a "real world" application you wouldn't use this function, but you would
	// use a semaphore (a pointer to which goes in the above VkSubmitInfo struct)
	// that will get signalled once the queue completes all the previous commands.
	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);


	/*
	 * Now that initialization is complete, we can start our program's event loop!
	 *
	 * We'll process the SDL's events, and then send the drawing/present commands.
	 * (in this demo we just clear the screen and present)
	 */

	SDL_Event sdlEvent;
	bool quit = false;

	while(!quit)
	{
		while(SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT) {
				quit = true;
			}
			if (sdlEvent.type == SDL_KEYDOWN && sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
				quit = true;
			}
			/*if (sdlEvent.type == SDL_MOUSEBUTTONDOWN){
				quit = true;
			}*/
		}

		if(!quit)
			quit = !renderSingleFrame(myDevice, myQueue, mySwapchain, myCmdBufferPresent, mySwapchainImagesVector, myPresentFence);

		//std::this_thread::sleep_for(std::chrono::milliseconds(14));	// FIXME hack to not spin at 100% cpu.
	}

	result = vkQueueWaitIdle(myQueue);
	assert(result == VK_SUCCESS);

	/*
	 * Deinitialization
	 */
	vkDestroyFence(myDevice, myPresentFence, nullptr);

	// You don't need to call vkFreeCommandBuffers for all command buffers; all command buffers
	// allocated from a command pool are released when the command pool is destroyed.
	vkDestroyCommandPool(myDevice, myCommandPool, nullptr);

	for(auto imgView : mySwapchainImageViewsVector)
		vkDestroyImageView(myDevice, imgView, nullptr);

	// NOTE! DON'T destroy the swapchain images, because they are already destroyed
	// during the destruction of the swapchain.
	vkDestroySwapchainKHR(myDevice, mySwapchain, nullptr);

	// There's no function for destroying a queue; all queues of a particular
	// device are destroyed when the device is destroyed.
	vkDestroyDevice(myDevice, nullptr);
	vkDestroySurfaceKHR(myInstance, mySurface, nullptr);
	destroyDebugReportCallback(myInstance, myDebugReportCallback);
	vkDestroyInstance(myInstance, nullptr);

	// Quit SDL
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
