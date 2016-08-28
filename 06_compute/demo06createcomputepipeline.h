#ifndef DEMO06CREATECOMPUTEPIPELINE_H
#define DEMO06CREATECOMPUTEPIPELINE_H

#include "../00_commons/00_utils.h"

#include <vulkan/vulkan.h>
#include <string>
#include <cassert>


/**
 * Create the compute VkPipeline for Demo 06.
 */
bool demo06CreateComputePipeline(const VkDevice theDevice,
                                 const VkPipelineLayout thePipelineLayout,
                                 const std::string & computeShaderFilename,
                                 VkPipeline & outPipeline
                                 )
{
	VkResult result;

	/*
	 * Load the shaders and create the VkShaderModules.
	 */
	VkShaderModule computeShaderModule;
	if(!vkdemos::utils::loadAndCreateShaderModule(theDevice, computeShaderFilename, computeShaderModule)) {
		std::cout << "!!! ERROR: couldn't create compute shader module." << std::endl;
		return false;
	}


	/*
	 * Specify the pipeline's shader stages.
	 */
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo =
	{
		.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext  = nullptr,
		.flags  = 0,
		.stage  = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = computeShaderModule,
		.pName  = "main",
		.pSpecializationInfo = nullptr,
	};

	/*
	 * Create the pipeline.
	 */
	VkComputePipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = shaderStageCreateInfo,
		.layout = thePipelineLayout,
		.basePipelineHandle = VK_NULL_HANDLE, // This and the following parameter are used
		.basePipelineIndex = -1,              //  to create derivative pipelines -- not used in this demo.
	};

	VkPipeline myComputePipeline;
	result = vkCreateComputePipelines(theDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &myComputePipeline);
	assert(result == VK_SUCCESS);

	vkDestroyShaderModule(theDevice, computeShaderModule, nullptr);

	outPipeline = myComputePipeline;
	return true;
}

#endif
