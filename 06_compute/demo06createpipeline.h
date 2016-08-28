#ifndef DEMO06CREATEPIPELINE_H
#define DEMO06CREATEPIPELINE_H

#include "../00_commons/00_utils.h"

#include <vulkan/vulkan.h>
#include <string>
#include <cassert>


/*
 * This struct represents the format of the vertices' data in memory.
 */
struct Demo06Vertex {
	float x, y;
};



/**
 * Create the Graphics VkPipeline for Demo 06.
 *
 * For an explanation of the various fields and structs, refer to Demo 02.
 */
bool demo06CreatePipeline(const VkDevice theDevice,
                          const VkRenderPass theRenderPass,
                          const VkPipelineLayout thePipelineLayout,
                          const std::string & vertexShaderFilename,
                          const std::string & fragmentShaderFilename,
                          const uint32_t vertexInputBinding,
                          VkPipeline & outPipeline
                          )
{
	VkResult result;

	/*
	 * Load the shaders and create the VkShaderModules.
	 */
	VkShaderModule vertexShaderModule, fragmentShaderModule;
	bool b1, b2;
	b1 = vkdemos::utils::loadAndCreateShaderModule(theDevice, vertexShaderFilename, vertexShaderModule);
	b2 = vkdemos::utils::loadAndCreateShaderModule(theDevice, fragmentShaderFilename, fragmentShaderModule);

	if(!b1 || !b2) {
		std::cout << "!!! ERROR: couldn't create shader modules." << std::endl;
		return false;
	}


	/*
	 * Specify the pipeline's shader stages.
	 */
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] =
	{
		[0] = {    // Vertex shader
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext  = nullptr,
			.flags  = 0,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName  = "main",
			.pSpecializationInfo = nullptr,
		},
		[1] = {    // Fragment shader
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext  = nullptr,
			.flags  = 0,
			.stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName  = "main",
			.pSpecializationInfo = nullptr,
		},
	};


	/*
	 * Specify parameters for vertex input.
	 */
	VkVertexInputBindingDescription vertexInputBindingDescription = {
		.binding = vertexInputBinding,
		.stride = sizeof(Demo06Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	// Attribute Description
	constexpr int inputAttributeDescCount = 1;
	VkVertexInputAttributeDescription vertexInputAttributeDescription[inputAttributeDescCount] =
	{
		[0] = {
			.location = 0,
			.binding = vertexInputBinding,
			.format = VK_FORMAT_R32G32_SFLOAT,
			.offset = offsetof(Demo06Vertex, x),
		},
	};

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingDescription,
		.vertexAttributeDescriptionCount = inputAttributeDescCount,
		.pVertexAttributeDescriptions = vertexInputAttributeDescription,
	};


	/*
	 * Specify parameters for input assembly.
	 */
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};


	/*
	 * Specify pipeline's dynamic state.
	 */
	VkDynamicState dynamicStateEnables[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStateEnables,
	};


	/*
	 * Specify the viewport state.
	 *
	 * We'll use dynamic viewport and scissor state, so we specify nullptr
	 * for these values.
	 */
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = nullptr,
		.scissorCount = 1,     // Must match viewportCount
		.pScissors = nullptr,
	};


	/*
	 * Specifiy rasterization parameters.
	 */
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0,
		.depthBiasClamp = 0,
		.depthBiasSlopeFactor = 0,
		.lineWidth = 1,
	};


	/*
	 * Specify blending parameters.
	 * (no blending in this demo)
	 */
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.logicOpEnable = VK_FALSE,                    // unused in this demo
		.logicOp = VK_LOGIC_OP_CLEAR,                 // (ignored)
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},   // (ignored if blending not enabled)
	};


	/*
	 * Specify depth buffer and stencil buffer parameters.
	 */
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {
			.failOp = VK_STENCIL_OP_KEEP,
			.passOp = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.compareMask = 0,
			.writeMask = 0,
			.reference = 0,
		},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 0.0f,
	};

	depthStencilStateCreateInfo.back = depthStencilStateCreateInfo.front;


	/*
	 * Specify Multisample parameters.
	 *
	 */
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,  // Only one sample per pixel (effectively disables multisampling)
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 0,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};


	/*
	 * Finally, create the pipeline with all the information we defined before.
	 *
	 */
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stageCount = 2,
	    .pStages = shaderStageCreateInfo,
		.pVertexInputState = &vertexInputStateCreateInfo,
		.pInputAssemblyState = &inputAssemblyStateCreateInfo,
		.pTessellationState = nullptr,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizationStateCreateInfo,
		.pMultisampleState = &multisampleStateCreateInfo,
		.pDepthStencilState = &depthStencilStateCreateInfo,
		.pColorBlendState = &colorBlendStateCreateInfo,
		.pDynamicState = &dynamicStateCreateInfo,
		.layout = thePipelineLayout,
		.renderPass = theRenderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // This and the following parameter are used
		.basePipelineIndex = -1,              //  to create derivative pipelines -- not used in this demo.
	};

	VkPipeline myGraphicsPipeline;
	result = vkCreateGraphicsPipelines(theDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &myGraphicsPipeline);
	assert(result == VK_SUCCESS);

	vkDestroyShaderModule(theDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(theDevice, fragmentShaderModule, nullptr);

	outPipeline = myGraphicsPipeline;
	return true;
}


#endif
