Demo 00: Common Functions
=========================

This directory contains header files containing one or more functions, implementing object creation or initialization routines; they are useful to all the other demos, so that common code is not rewritten every time. Moreover, every single function is also useful in isolation, so one can reuse them in their project without many dependencies on the rest of the demo's code.

All the functions are inside the "vkdemos" namespace, except for file `00_utils.h` where functions are under the "vkdemos::utils" namespace.

File and function list:
-----------------------

- 00_utils.h

	- `VkResultToString`: converts a VkResult into its ASCII string representation.
	- `sdl2Initialization`: executes SDL2 initialization and creates a window.
	- `findMemoryTypeWithProperties`: Search a memory type with the required properties from a VkPhysicalDeviceMemoryProperties object.
	- `createFence`: Utility function to create a VkFence on a specified VkDevice.
	- `createSemaphore`: Utility function to create a VkSemaphore on a specified VkDevice.
	- `createFramebuffer`: Utility function to create a VkFramebuffer object from a set of VkImageViews.
	- `loadAndCreateShaderModule`: Loads a SPIR-V shader from file and creates a VkShaderModule from it.

- 01_createVkInstance.h

	- `createVkInstance`: creates a VkInstance with the specified extensions and layers enabled.

- 02_debugReportCallback.h

	- `debugCallback`: a basic debug callback that logs to stdout any messages it receives.
	- `createDebugReportCallback`: creates a VkDebugReportCallbackEXT on a specified VkInstance.
	- `destroyDebugReportCallback`: destroys a VkDebugReportCallbackEXT previously created.

- 03_createVkSurface.h

	- `createVkSurface`: creates a VkSurface from a SDL_SysWMinfo context.
	- `createVkSurfaceXCB`: creates a VkSurface from an XCB connection and window.

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

- 08_createAndAllocateImage.h

	- `createAndAllocateImage`: Creates a VkImage and allocates memory for it.

- 09_createAndAllocateBuffer.h

	- `createAndAllocateBuffer`: Creates a VkBuffer and allocates memory for it.

- 10_submitimagebarrier.h

	- `submitImageBarrier`: Appends a CmdPipelineBarrier with an Image Barrier operation to a command buffer.

- 11_loadimagefromfile.h

	- `loadImageFromFile`: Load an RGBA image from a specified path.

