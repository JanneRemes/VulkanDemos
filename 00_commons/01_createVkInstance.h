#ifndef VKDEMOS_CREATEVKINSTANCE_H
#define VKDEMOS_CREATEVKINSTANCE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <algorithm>
#include <cstring>

#include "00_utils.h"

namespace vkdemos {

/**
 * Creates a VKInstance that has all the layer names in layerNamesToEnable
 * and all the extension names in extensionNamesToEnable enabled.
 */
bool createVkInstance(const std::vector<const char *> & layerNamesToEnable,
                      const std::vector<const char *> & extensionNamesToEnable,
                      const char * applicationName,
                      const char * engineName,
                      VkInstance & outInstance)
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
		.apiVersion = VK_MAKE_VERSION(1, 0, 3), // Vulkan version the application expects to use;
		                                        // if = 0, this field is ignored; otherwise, if the implementation
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

	instanceCreateInfo.enabledLayerCount       = (uint32_t)layerNamesToEnable.size();
	instanceCreateInfo.ppEnabledLayerNames     = layerNamesToEnable.data();
	instanceCreateInfo.enabledExtensionCount   = (uint32_t)extensionNamesToEnable.size();
	instanceCreateInfo.ppEnabledExtensionNames = extensionNamesToEnable.data();

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
		std::cout << "--- Found " << layerPropertiesVector.size() << " instance layers:" << std::endl;
		std::cout << "    |                   Name                  |   Spec   |   Impl   | Description" << std::endl;
		std::cout << "    +-----------------------------------------+----------+----------+------------" << std::endl;

		for(const auto & layer : layerPropertiesVector)
		{
			std::cout <<  "    | " << std::left  << std::setw(40) << layer.layerName
			          <<  "| " << std::right << std::setw(8) << layer.specVersion
			          << " | " << std::setw(8) << layer.implementationVersion
			          << " | " << layer.description
			          << std::endl;
		}
		std::cout << "    +-----------------------------------------+----------+----------+------------\n" << std::endl;
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
		std::cout << "--- Found " << extPropertiesVector.size() << " extensions for layer " << (layerName==nullptr ? "(null)" : layerName) << std::endl;

		if(extPropertiesVector.size() > 0)
		{
			std::cout << "    |               Name               |  Spec  |" << std::endl;
			std::cout << "    +----------------------------------+--------+" << std::endl;

			for(const auto & ext : extPropertiesVector) {
				std::cout << "    | " << std::left << std::setw(33) << ext.extensionName
						  << "| " << std::right << std::setw(6) << ext.specVersion
						  << " |"
						  << std::endl;
			}
			std::cout << "    +----------------------------------+--------+\n";
		}
		std::cout << std::endl;

		return extPropertiesVector;
	};

	// Passing a null pointer as the name of the layer, we get the Vulkan implementation's (global) extensions,
	// and the extensions of the implicitly enabled layers.
	auto globalExtensionsVector = queryAndPrintExtensions(nullptr);

	for(const auto & layer : layerPropertiesVector)
		queryAndPrintExtensions(layer.layerName);

	std::cout << std::endl;


	/*
	 * Layers enable:
	 * Check if all the layer names we want to enable are present
	 * in the VkLayerProperties we collected before in layerPropertiesVector.
	 */
	for(const auto & layerName : layerNamesToEnable)
	{
		auto itr = std::find_if(layerPropertiesVector.begin(), layerPropertiesVector.end(),
			[&](const VkLayerProperties & extProp){
				return strcmp(layerName, extProp.layerName) == 0;
			}
		);

		if(itr == layerPropertiesVector.end()) {
			std::cout << "!!! ERROR: Layer " << layerName << " was not found." << std::endl;
			return false;
		}
	}


	/*
	 * Extensions enable:
	 * Check if all the extension names we want to enable are present
	 * in the VkExtensionProperties we collected before in globalExtensionsVector.
	 */
	for(const auto & extName : extensionNamesToEnable)
	{
		auto itr = std::find_if(globalExtensionsVector.begin(), globalExtensionsVector.end(),
			[&](const VkExtensionProperties & extProp){
				return strcmp(extName, extProp.extensionName) == 0;
			}
		);

		if(itr == globalExtensionsVector.end()) {
			std::cout << "!!! ERROR: extension " << extName << " was not found in the global extensions vector." << std::endl;
			return false;
		}
	}


	/*
	 * Here the magic happens: the VkInstance gets created from
	 * the structs we filled before.
	 */
	VkInstance myInstance;
	result = vkCreateInstance(&instanceCreateInfo, nullptr, &myInstance);

	if(result != VK_SUCCESS) {
		std::cout << "!!! ERROR: Cannot create Vulkan instance, " << vkdemos::utils::VkResultToString(result) << std::endl;
		return false;
	}

	std::cout << "+++ VkInstance created succesfully!\n" << std::endl;
	outInstance = myInstance;
	return true;
}

}	// vkdemos

#endif
