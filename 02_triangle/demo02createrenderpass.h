#ifndef DEMO02CREATERENDERPASS_H
#define DEMO02CREATERENDERPASS_H

#include <vulkan/vulkan.h>
#include <cassert>

/**
 * Create the renderpass for this demo.
 */
bool demo02CreateRenderPass(const VkDevice theDevice,
                                  const VkFormat theSwapchainImagesFormat,
                                  const VkFormat theDepthBufferFormat,
                                  VkRenderPass & outRenderPass)
{
	VkResult result;

	/*
	 * We need to specify all the attachments that will be used inside
	 * this renderpass.
	 * For each attachment we specify its format, the number of samples,
	 * what operations to do when reading and writing on the attachment,
	 * the behaviour of the stencil buffer, and the initial and final
	 * layout of the attachment image.
	 *
	 * A nice functionality of renderpasses is that we can specify
	 * the initial layout of the attachment before it enters the render pass,
	 * and the layout we expect to see after the render pass finishes.
	 * This way the driver will insert the appropriate layout change operations for us,
	 * with all the correct barriers in place!
	 */
	const VkAttachmentDescription attachmentDescription[2] = {
		[0] = {
			.flags = 0,
			.format = theSwapchainImagesFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		},
		[1] = {
			.flags = 0,
			.format = theDepthBufferFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};


	/*
	 * A renderpass is composed by one or more subpasses.
	 *
	 * Subpasses represent a part of the overall renderpass rendering:
	 * they read from a set of input attachments and write their results in
	 * one or more color attachments, and optionally a depth and a stencil attachment.
	 * They also take a list of attachments in the renderpass that will be preserved
	 * throughout the execution of the subpass; with these informations, the driver can
	 * schedule the execution of the subpasses' rendering operations in the best way possible
	 * because it knows the full dependency graph between the subpasses.
	 *
	 * A renderpass must have at least one subpass.
	 */

	// We need to tell the subpass what color attachment and what depth attachment
	// it must use: we do this with VkAttachmentReferences.
	const VkAttachmentReference colorAttachmentReference = {
		.attachment = 0,              // This is the index of the attachment in the attachmentDescription vector defined above.
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	const VkAttachmentReference depthAttachmentReference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	// We define the subpass here
	const VkSubpassDescription subpassDescription = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.flags = 0,
		.inputAttachmentCount = 0,                             // No input attachments
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,                             // A single color output attachment
		.pColorAttachments = &colorAttachmentReference,
		.pResolveAttachments = nullptr,                        // No resolve attachments
		.pDepthStencilAttachment = &depthAttachmentReference,  // A depth/stencil attachment
		.preserveAttachmentCount = 0,                          // We don't have any other attachments to preserve
		.pPreserveAttachments = nullptr,
	};


	/*
	 * Create the VkRenderPass object.
	 *
	 * The dependencyCount and pDependencies fields define a list
	 * of additional dependencies between pairs of subpasses
	 * in this renderpass; basically it's like an additional
	 * pipeline barrier between subpasses.
	 */
	const VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.attachmentCount = 2,
		.pAttachments = attachmentDescription, // All the attachments needed for all the subpasses.
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,     // List of subpasses in this renderpass.
		.dependencyCount = 0,
		.pDependencies = nullptr,
	};

	VkRenderPass myRenderPass;
	result = vkCreateRenderPass(theDevice, &renderPassCreateInfo, nullptr, &myRenderPass);
	assert(result == VK_SUCCESS);

	outRenderPass = myRenderPass;
	return true;
}


#endif
