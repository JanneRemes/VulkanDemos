#ifndef DEMO06FILLRENDERINGCOMMANDBUFFER_H
#define DEMO06FILLRENDERINGCOMMANDBUFFER_H

#include "pushconstdata.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <cassert>

/**
 * Fill the specified command buffer with the present commands for this demo.
 */
bool demo06FillRenderingCommandBuffer(const VkCommandBuffer theCommandBuffer,
                                      const VkFramebuffer theCurrentFramebuffer,
                                      const VkRenderPass theRenderPass,
                                      const VkPipeline thePipeline,
                                      const VkPipelineLayout thePipelineLayout,
                                      const VkBuffer theVertexBuffer,
                                      const uint32_t vertexInputBinding,
                                      const uint32_t numberOfVertices,
                                      const VkDescriptorSet theDescriptorSet,
                                      const int width,
                                      const int height,
                                      const PushConstData & pushConstData
                                      )
{
	VkResult result;

	/*
	 * Begin recording of the command buffer
	 */
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr,
	};

	result = vkBeginCommandBuffer(theCommandBuffer, &commandBufferBeginInfo);
	assert(result == VK_SUCCESS);

	/*
	 * Record the state setup and drawing commands.
	 */
	// Begin the renderpass (passing also the clear values for all the attachments).
	const VkClearValue clearValues[2] = {
		[0] = {.color.float32 = {0.15f, 0.15f, 0.15f, 1.0f}}, // Clear color for the color attachment at index 0
		[1] = {.depthStencil  = {1.0f, 0}},                   // Clear value for the depth buffer at attachment index 1
	};

	const VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = theRenderPass,
		.framebuffer = theCurrentFramebuffer,
	    .renderArea.offset = {0, 0},
	    .renderArea.extent = {(uint32_t)width, (uint32_t)height},
		.clearValueCount = 2,
		.pClearValues = clearValues,
	};

	vkCmdBeginRenderPass(theCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// Bind the pipeline.
	vkCmdBindPipeline(theCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, thePipeline);

	// Set the viewport dynamic state.
	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)width,
		.height = (float)height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	vkCmdSetViewport(theCommandBuffer, 0, 1, &viewport);

	// Set the scissor dynamic state.
	VkRect2D scissor = {
		.offset.x = 0,
		.offset.y = 0,
		.extent.width = (uint32_t)width,
		.extent.height = (uint32_t)height,
	};

	vkCmdSetScissor(theCommandBuffer, 0, 1, &scissor);

	// Bind the vertex buffer.
	VkDeviceSize buffersOffsets = 0;
	vkCmdBindVertexBuffers(theCommandBuffer, vertexInputBinding, 1, &theVertexBuffer, &buffersOffsets);

	/*
	 * Bind the descriptor set.
	 */
	vkCmdBindDescriptorSets(
		theCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		thePipelineLayout,
	    0,                 // firstSet
		1,                 // descriptorSetCount
		&theDescriptorSet, // pDescriptorSets
		0,                 // dynamicOffsetCount
		nullptr            // pDynamicOffsets
	);

	// Send the Push Constants.
	vkCmdPushConstants(
		theCommandBuffer,
		thePipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,  // shader stages that will use the push constants
		0,                      // push constant offset (as defined in the push constants range in the pipeline layout)
		sizeof(PushConstData),  // length of push constants data
		&pushConstData          // pointer to push constants data
	);

	// Send the draw command, that will begin all the rendering magic
	vkCmdDraw(theCommandBuffer, numberOfVertices, 1, 0, 0);

	// End the render pass commands.
	vkCmdEndRenderPass(theCommandBuffer);

	/*
	 * End recording of the command buffer
	 */
	result = vkEndCommandBuffer(theCommandBuffer);
	return true;
}

#endif
