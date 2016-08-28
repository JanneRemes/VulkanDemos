#ifndef DEMO06CREATEVKDEVICEANDVKQUEUES_H
#define DEMO06CREATEVKDEVICEANDVKQUEUES_H

#include "vulkan/vulkan.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstring>

/**
 * Demo 06: Creates a VkDevice, a graphics VkQueue and a compute VkQueue.
 *
 * For more details, see 00_commons/05_createVkDeviceAndVkQueue.h
 */
bool demo06createVkDeviceAndVkQueues(const VkPhysicalDevice thePhysicalDevice,
                                     const VkSurfaceKHR theSurface,
                                     const std::vector<const char *> & layersNamesToEnable,
                                     VkDevice & outDevice,
                                     VkQueue & outGraphicsQueue,
                                     uint32_t & outGraphicsQueueFamilyIndex,
                                     VkQueue & outComputeQueue,
                                     uint32_t & outComputeQueueFamilyIndex
                                     )
{
	VkResult result;

	/*
	 * Check physical device for extensions
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
	 * Find appropriate queue families
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
	int indexOfComputeQueueFamily = -1;
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

		if(queueFamProp.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			if(indexOfComputeQueueFamily < 0)
				indexOfComputeQueueFamily = queueFamilyIndex;
		}

		queueFamilyIndex++;
	}


	if(indexOfGraphicsQueueFamily < 0) {
		std::cout << "!!! ERROR: chosen physical device has no queue families that support both graphics and present!" << std::endl;
		return false;
	}

	if(indexOfComputeQueueFamily < 0) {
		std::cout << "!!! ERROR: chosen physical device has no queue families that support compute!" << std::endl;
		return false;
	}

	/*
	 * Queue create info
	 */
	std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfoVector;

	VkDeviceQueueCreateInfo qciToFill;
	qciToFill.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	qciToFill.pNext = nullptr;
	qciToFill.flags = 0;

	float queuePriority = 1.0f;
	qciToFill.queueFamilyIndex = (uint32_t)indexOfGraphicsQueueFamily;
	qciToFill.queueCount = (indexOfGraphicsQueueFamily != indexOfComputeQueueFamily) ? 1 : 2;
	qciToFill.pQueuePriorities = &queuePriority;
	deviceQueueCreateInfoVector.push_back(qciToFill);

	if(indexOfGraphicsQueueFamily != indexOfComputeQueueFamily) {
		qciToFill.queueFamilyIndex = (uint32_t)indexOfComputeQueueFamily;
		deviceQueueCreateInfoVector.push_back(qciToFill);
	}

	/*
	 * Physical device features
	 */
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(thePhysicalDevice, &physicalDeviceFeatures);

	/*
	 * Device creation
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
	 * Get queues
	 */
	VkQueue myGraphicsQueue, myComputeQueue;

	vkGetDeviceQueue(myDevice,
		(uint32_t)indexOfGraphicsQueueFamily,
		0,
		&myGraphicsQueue
	);

	vkGetDeviceQueue(myDevice,
		(uint32_t)indexOfComputeQueueFamily,
		(indexOfGraphicsQueueFamily != indexOfComputeQueueFamily) ? 0 : 1,
		&myComputeQueue
	);

	std::cout << "\n+++ VkDevice and VkQueues created succesfully!\n" << std::endl;

	outDevice = myDevice;
	outGraphicsQueue = myGraphicsQueue;
	outGraphicsQueueFamilyIndex = (uint32_t)indexOfGraphicsQueueFamily;
	outComputeQueue = myComputeQueue;
	outComputeQueueFamilyIndex = (uint32_t)indexOfComputeQueueFamily;
	return true;
}

#endif
