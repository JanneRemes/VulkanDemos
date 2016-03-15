#ifndef VKDEMOS_CHOOSEVKPHYSICALDEVICE_H
#define VKDEMOS_CHOOSEVKPHYSICALDEVICE_H

#include "vulkan/vulkan.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <iomanip>

namespace vkdemos {

/**
 * Query the instance for all the physical devices, and return one of them.
 * (in this demo, for simplicity, we take the first phydev we find;
 * ideally, we would query the capabilities of each phydev and choose the
 * one best suited for our needs).
 */
bool chooseVkPhysicalDevice(const VkInstance theInstance, const unsigned int deviceToUseIndex, VkPhysicalDevice & outPhysicalDevice)
{
	VkResult result;

	/*
	 * Here we query the Vulkan Implementation for the physical devices
	 * available in the system. We then log their information to the console.
	 */
	uint32_t physicalDevicesCount = 0;
	result = vkEnumeratePhysicalDevices(theInstance, &physicalDevicesCount, nullptr);
	assert(result == VK_SUCCESS && physicalDevicesCount > 0);

	std::vector<VkPhysicalDevice> physicalDevicesVector(physicalDevicesCount);
	result = vkEnumeratePhysicalDevices(theInstance, &physicalDevicesCount, physicalDevicesVector.data());
	assert(result == VK_SUCCESS);


	// For each physical device we query its properties and pretty-print them to the console.
	int deviceIndex = 0;
	for(const auto & phyDev : physicalDevicesVector)
	{
		VkPhysicalDeviceProperties phyDevProperties;
		vkGetPhysicalDeviceProperties(phyDev, &phyDevProperties);

		std::cout << "--- Found physical device: " << phyDevProperties.deviceName << " (index: " << (deviceIndex++) << ")" << std::endl;
		std::cout << "        apiVersion: "  << phyDevProperties.apiVersion << std::endl;
		std::cout << "     driverVersion: "  << phyDevProperties.driverVersion << std::endl;
		std::cout << "          vendorID: "  << phyDevProperties.vendorID << std::endl;
		std::cout << "          deviceID: "  << phyDevProperties.deviceID << std::endl;
		std::cout << "        deviceType: (" << phyDevProperties.deviceType << ") ";

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

	std::cout << "\n+++ Index of physical device choosen: " << deviceToUseIndex << '\n' << std::endl;

	outPhysicalDevice = physicalDevicesVector[deviceToUseIndex];
	return true;
}

}

#endif
