#ifndef VKDEMOS_CREATEVKDEVICEANDVKQUEUE_H
#define VKDEMOS_CREATEVKDEVICEANDVKQUEUE_H

#include "vulkan/vulkan.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstring>

namespace vkdemos {

/**
 * Creates a VkDevice and its associated VkQueue.
 */
bool createVkDeviceAndVkQueue(const VkPhysicalDevice thePhysicalDevice, const VkSurfaceKHR theSurface, const std::vector<const char *> & layersNamesToEnable, VkDevice & outDevice, VkQueue & outQueue, uint32_t & outQueueFamilyIndex)
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
		std::cout << "!!! ERROR: chosen physical device does not support VK_KHR_SWAPCHAIN_EXTENSION_NAME!" << std::endl;
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
		std::cout << "!!! ERROR: chosen physical device has no queue families!" << std::endl;
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

		std::cout << "--- Properties for queue family " << queueFamilyIndex << std::endl;
		std::cout << "                     queueFlags:";

		if(queueFamProp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			std::cout << " GRAPHICS";
		if(queueFamProp.queueFlags & VK_QUEUE_COMPUTE_BIT)
			std::cout << " COMPUTE";
		if(queueFamProp.queueFlags & VK_QUEUE_TRANSFER_BIT)
			std::cout << " TRANSFER";
		if(queueFamProp.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			std::cout << " SPARSE_BINDING";

		std::cout << '\n';
		std::cout << "                     queueCount: " << queueFamProp.queueCount << std::endl;
		std::cout << "             timestampValidBits: " << queueFamProp.timestampValidBits << std::endl;
		std::cout << "    minImageTransferGranularity: " << queueFamProp.minImageTransferGranularity.width
		                                        << ", " << queueFamProp.minImageTransferGranularity.height
		                                        << ", " << queueFamProp.minImageTransferGranularity.depth
		                                        << std::endl;

		std::cout << "       Does it support present?: " << std::boolalpha << bool(doesItSupportPresent) << std::endl;

		// Select queue family if it supports all the requisites.
		if(bool(queueFamProp.queueFlags & VK_QUEUE_GRAPHICS_BIT) && doesItSupportPresent == VK_TRUE) {
			if(indexOfGraphicsQueueFamily < 0)
				indexOfGraphicsQueueFamily = queueFamilyIndex;
		}

		queueFamilyIndex++;
	}

	if(indexOfGraphicsQueueFamily < 0) {
		std::cout << "!!! ERROR: chosen physical device has no queue families that support both graphics and present!" << std::endl;
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

	std::vector<const char *> extensionNamesToEnable = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo deviceCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .queueCreateInfoCount    = (uint32_t)deviceQueueCreateInfoVector.size(),
	    .pQueueCreateInfos       = deviceQueueCreateInfoVector.data(),
	    .enabledLayerCount       = (uint32_t)layersNamesToEnable.size(),
	    .ppEnabledLayerNames     = layersNamesToEnable.data(),
	    .enabledExtensionCount   = (uint32_t)extensionNamesToEnable.size(),
	    .ppEnabledExtensionNames = extensionNamesToEnable.data(),
	    .pEnabledFeatures        = &physicalDeviceFeatures
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

}

#endif
