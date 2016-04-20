Demo 02: Triangle
=================

This demo builds upon Demo 01, and shows how to render a simple triangle: it performs all the initialization commands as Demo 01, and creates a depth buffer and as many VkFramebuffers as there are swapchain images.

It then creates a buffer to be used as a vertex buffer, and copies data to it. A VkPipeline and a VkRenderpass are then created with the appropriate parameters so that it can proceed to draw the triangle to the screen.

