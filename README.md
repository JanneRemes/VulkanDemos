Vulkan Demos and Tutorials
==========================

This repository is a collection of simple applications using the Vulkan API.
The source code is heavily commented and structured so that a beginner can start learning the basics of Vulkan.

The language used is C++11/14. All the code is available under the MIT license.

The code currently has been tested only on a Nvidia GTX 760 graphics card, using the experimental Vulkan drivers (Nvidia 355) on Linux Mint 17.3.
The only windowing system currently supported is XCB; patches are welcome!

Demo list:
----------

- **00 - Commons**

  Not really a demo: it's a collection of functions common to all the other demos (object creation, initializations etc).

- **01 - Clearscreen**

  This demo shows how to create a window using SDL2, create a Vulkan Instance, Physical Device, Device, Queue, Swapchain, Command Pool, Command Buffer, and submit commands to clear the screen with a specified RGB color.

