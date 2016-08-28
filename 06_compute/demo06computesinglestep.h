#ifndef DEMO06COMPUTESINGLESTEP_H
#define DEMO06COMPUTESINGLESTEP_H

#include "pushconstdata.h"

#include <vulkan/vulkan.h>
#include <cassert>


struct PerComputeData
{
	VkCommandBuffer computeCmdBuffer;
	VkSemaphore computeCompletedSemaphore;
	VkFence computeFence;
	bool fenceInitialized;
};


static constexpr int WORKGROUP_WIDTH = 16;
static constexpr int WORKGROUP_HEIGHT = 16;

/**
 * Sends commands to the GPU to compute a single step of the simulation.
 * Returns true on success and false on failure.
 */
bool demo06ComputeSingleStep(const VkDevice theDevice,
                             const VkQueue theQueue,
                             const VkPipeline thePipeline,
                             const VkPipelineLayout thePipelineLayout,
                             const VkDescriptorSet theDescriptorSet,
                             PerComputeData & thePerComputeData,
                             const int arenaWidth,
                             const int arenaHeight,
                             const PushConstData & pushConstData
                             )
{
	VkResult result;
	VkCommandBuffer & theCommandBuffer = thePerComputeData.computeCmdBuffer;

	// Wait for the fence before reusing the command buffer
	if(thePerComputeData.fenceInitialized) {
		vkWaitForFences(theDevice, 1, &thePerComputeData.computeFence, VK_TRUE, UINT64_MAX);
		vkResetFences(theDevice, 1, &thePerComputeData.computeFence);
	}

	thePerComputeData.fenceInitialized = true;

	// Begin recording of the command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr,
	};

	result = vkBeginCommandBuffer(theCommandBuffer, &commandBufferBeginInfo);
	assert(result == VK_SUCCESS);

	// Bind the pipeline.
	vkCmdBindPipeline(theCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, thePipeline);

	// Bind the descriptor set.
	vkCmdBindDescriptorSets(
		theCommandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
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
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		sizeof(PushConstData),
		&pushConstData
	);

	// Send the draw command, that will begin all the rendering magic
	vkCmdDispatch(theCommandBuffer, arenaWidth/WORKGROUP_WIDTH, arenaHeight/WORKGROUP_HEIGHT, 1);

	// End recording of the command buffer
	result = vkEndCommandBuffer(theCommandBuffer);

	/*
	 * Submit the present command buffer to the queue.
	 */
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &theCommandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &thePerComputeData.computeCompletedSemaphore
	};

	result = vkQueueSubmit(theQueue, 1, &submitInfo, thePerComputeData.computeFence);
	assert(result == VK_SUCCESS);

	return true;
}

#endif // DEMO06COMPUTESINGLESTEP_H
