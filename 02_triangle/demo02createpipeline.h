#ifndef DEMO02CREATEPIPELINE_H
#define DEMO02CREATEPIPELINE_H

#include "../00_commons/00_utils.h"

#include <vulkan/vulkan.h>
#include <string>
#include <cassert>


/*
 * This struct represents the format of the vertice's data in memory.
 */
struct TriangleDemoVertex {
	float x, y, z;
	float r, g, b;
};



/**
 * Create the VkPipeline for this demo.
 *
 * Pipelines are "the things that do stuff": they are a path
 * inside the GPU that vertex and fragment data follow
 * and they define standardized processing stages that transform said data;
 * VkPipeline objects define how those stages are configured.
 *
 * For this demo, we define a single graphics pipeline, with a vertex shader
 * and a fragment shader; there is no input data other than a vertex buffer,
 * and the pipeline outputs to an RGBA color attachment and a depth buffer.
 */
bool demo02CreatePipeline(const VkDevice theDevice,
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
	 * Specify the pipeline's stages.
	 *
	 * In this demo we have 2 stages: the vertex shader stage, and the fragment shader stage
	 * (other optional stages are tessellation and geometry shaders).
	 */
	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {
		[0] = {    // Vertex shader
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext  = nullptr,
			.flags  = 0,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName  = "main",               // Shader entry point name (i.e. the function that will be called when shader's execution starts)
			.pSpecializationInfo = nullptr,	// This field allows to specify constant values for constants in SPIR-V modules, before compiling the pipeline.
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
	 *
	 */
	// A Vertex Input Binding describes a binding point on the pipeline where a vertex buffer can be connected.
	VkVertexInputBindingDescription vertexInputBindingDescription = {
		.binding = vertexInputBinding,
		.stride = sizeof(TriangleDemoVertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	// Vertex Attribute Descriptions describe the format of the data expected on a buffer
	// attached to a particular Vertex Input Binding.
	// Here we have two Attribute Descriptions: a vector of 3 float elements for vertex position (x,y,z),
	// and a vector of 3 float elements for vertex color (r,g,b).
	VkVertexInputAttributeDescription vertexInputAttributeDescription[2] = {
		[0] = {
			.location = 0,
			.binding = vertexInputBinding,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(TriangleDemoVertex, x),
		},
		[1] = {
			.location = 1,
			.binding = vertexInputBinding,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(TriangleDemoVertex, r),
		},
	};

	// This will go in the Pipeline Create Info struct.
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &vertexInputBindingDescription,
		.vertexAttributeDescriptionCount = 2,
		.pVertexAttributeDescriptions = vertexInputAttributeDescription,
	};


	/*
	 * Specify parameters for input assembly.
	 *
	 * The Input Assembly stage is a fixed function stage in the pipeline
	 * that collects all the processed vertices and sends them to the rasterizer.
	 * We must tell it how to interpret those vertices: in this demo, our vertices
	 * describe a sequence of indipendent triangles.
	 */
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};


	/*
	 * Specify what parameters will be part of the dynamic state.
	 *
	 * Some of the pipeline's parameters are static and baked into the pipeline object
	 * at the instant of creation, but others can be chosen to be changed at a later time;
	 * those parameters are what constitute the "dynamic state" of the pipeline.
	 *
	 * You can choose what parameters to leave static and what to make dynamic;
	 * refer to Vulkan's specification to know what parameters can be made dynamic.
	 * Just be aware that dynamic states cannot be taken into consideration when creating
	 * the pipeline object, so there are less possibilities for the driver to apply various
	 * device-dependent optimizations.
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
	 *
	 * These parameters tell the rasterizer how to treat the polygons it receives,
	 * for example what the fill mode is (fill the triangles or draw only the borders),
	 * the culling mode (front or back face culling), etc.
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
	 *
	 * In this demo we don't enable blending; this means the fragments generated will overwrite
	 * whatever the framebuffer contains at that moment.
	 */

	// We need to create a VkPipelineColorBlendAttachmentState for each color attachment
	// we have on the subpass of the renderpass where this pipeline will be used.
	// (We have only one in this demo)
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
		.blendEnable = VK_FALSE,   // Disable blending for this demo. All the other values in this struct are ignored except colorWriteMask.
		.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		//    ^^^  Which components of the fragments to write out to memory (in this case, all of them).
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
	 *
	 * These parameters tell the GPU how to treat values in the depth and stencil buffers
	 * and what to do with with fragments that pass/fail the depth/stencil tests.
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
		.front = {               // These values are ignored since stencilTestEnable is false
			.failOp = VK_STENCIL_OP_KEEP,
			.passOp = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.compareMask = 0,
			.writeMask = 0,
			.reference = 0,
		},
		.minDepthBounds = 0.0f,  // (ignored because depthBoundsTestEnable is false)
		.maxDepthBounds = 0.0f,  // (ignored because depthBoundsTestEnable is false)
	};

	depthStencilStateCreateInfo.back = depthStencilStateCreateInfo.front;	// I'm lazy :)


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
