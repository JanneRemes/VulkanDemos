Demo 00: Common Functions
=========================

This directory contains header files containing one or more functions, implementing object creation or initialization routines; they are useful to all the other demos, so that common code is not rewritten every time. Moreover, every single function is also useful in isolation, so one can reuse them in their project without many dependencies on the rest of the demo's code.

All the functions are inside the "vkdemos" namespace, except for file `00_utils.h` where functions are under the "vkdemos::utils" namespace.

File and function list:
-----------------------

- 00_utils.h

	- `VkResultToString`: converts a VkResult into its ASCII string representation.

- 01_createVkInstance.h

	- `createVkInstance`: creates a VkInstance with the specified extensions and layers enabled.

- 02_debugReportCallback.h

	- `debugCallback`: a basic debug callback that logs to stdout any messages it receives.
	- `createDebugReportCallback`: creates a VkDebugReportCallbackEXT on a specified VkInstance.
	- `destroyDebugReportCallback`: destroys a VkDebugReportCallbackEXT previously created.

- 03_createVkSurface.h

	- `createVkSurfaceXCB`: creates a VkSurface from a XCB connection and window.

- 04_chooseVkPhysicalDevice.h

	- `chooseVkPhysicalDevice`: selects a VkPhysicalDevice from the available devices in the supplied VkInstance.

- 05_createVkDeviceAndVkQueue.h

	- `createVkDeviceAndVkQueue`: creates a VkDevice and related VkQueue from a VkPhysicalDevice.

- 06_swapchain.h

	- `createVkSwapchain`: creates a swapchain from a VkPhysicalDevice, VkDevice and VkSurfaceKHR.
	- `getSwapchainImagesAndViews`: extracts the VkImage-s from a VkSwapchainKHR, and creates their associated VkImageViews.

- 07_commandPoolAndBuffer.h

	- `createCommandPool`: creates a VkCommandPool from the specified VkDevice.
	- `allocateCommandBuffer`: creates a VkCommandBuffer from the specified VkDevice and VkCommandPool

