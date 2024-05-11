#include "pipeline.hpp"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cassert>

namespace vraus_VulkanEngine {
	Pipeline::Pipeline(
		Device& device, 
		const std::string& vertFilepath, 
		const std::string& fragFilepath, 
		const PipelineConfigInfo& configInfo
	) : device {device} {
		createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
	}

	Pipeline::~Pipeline()
	{
		vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
		vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
		vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
	}

	void Pipeline::bind(VkCommandBuffer commandBuffer)
	{
		// VK_PIPELINE_BIND_POINT_GRAPHICS : specifies binding as a graphics pipeline.
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	}

	PipelineConfigInfo Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height)
	{
		PipelineConfigInfo configInfo{};
		
		// First step of our pipeline
		configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; // Struct Type Member
		configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Specification to make triangles out of every 3 verticies (opti : strip)
		configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		// Configuring Viewport and Scissor
		// Viewport describes the transformation between our pipeline's output and our target image
		configInfo.viewport.x = 0.0f;
		configInfo.viewport.y = 0.0f;
		configInfo.viewport.width = static_cast<float>(width);
		configInfo.viewport.height = static_cast<float>(height);
		configInfo.viewport.minDepth = 0.0f;
		configInfo.viewport.maxDepth = 1.0f;

		// Scissor discard any pixel outside of the rectangle
		configInfo.scissor.offset = { 0,0 };
		configInfo.scissor.extent = { width, height };

		// Rasterization stage : Breaks up our geometry into fragments for each pixel our triangle overlaps
		configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; // Struct Type Member
		configInfo.rasterizationInfo.depthClampEnable = VK_FALSE; // Forces Z component of gl_Position to be between 0 and 1 : prefere FALSE (TRUE: Enable GPU Feature)
		configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		configInfo.rasterizationInfo.lineWidth = 1.0f;
		configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // Which side are we looking at depending on the order of the given points
		configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;
		configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // Optional
		configInfo.rasterizationInfo.depthBiasClamp = 0.0f; // Optional
		configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f; // Optional

		// Multisample relatre to how the rasterizer handles the edges of our geometry (MAA: Multisample Anti-Aliasing)
		configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; // Struct Type Member
		configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.multisampleInfo.minSampleShading = 1.0f;  // Optional
		configInfo.multisampleInfo.pSampleMask = nullptr; // Optional
		configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // Optional
		configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE; // Optional

		// Blending controls how we combine colors in our frame buffer
		configInfo.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
		configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

		configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		configInfo.colorBlendInfo.attachmentCount = 1;
		configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
		configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  // Optional
		configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  // Optional

		// Depth Buffer that stores a value for every pixel juste like our color attachemnt stores the color
		configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		configInfo.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.depthStencilInfo.front = {};  // Optional
		configInfo.depthStencilInfo.back = {};   // Optional

		return configInfo;
	}

	std::vector<char> Pipeline::readFile(const std::string& filepath) {
		/* bit flag explains
		* std::iod::ate - When the file opens, we seek the end imediatly. 
		*	(Makes the size a bit more conveniant)
		* std::ios::binary - Read the file as a binary.
		*	Avoid any unwanted text transformation from occuring.
		*/
		std::ifstream file{ filepath, std::ios::ate | std::ios::binary };

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filepath);
		}

		// Since we are at the end of the file, file.tellg() will give us the last position hense the file size.
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		file.close();
		return buffer;
	}

	void Pipeline::createGraphicsPipeline(const std::string& vertFilepath, const std::string& fragFilepath, const PipelineConfigInfo& configInfo)
	{
		assert(configInfo.pipelineLayout != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no pipelineLayout provided in configInfo");
		assert(configInfo.renderPass != VK_NULL_HANDLE && "Cannot create graphics pipeline:: no renderPass provided in configInfo");
		auto vertCode = readFile(vertFilepath);
		auto fragCode = readFile(fragFilepath);

		createShaderModule(vertCode, &vertShaderModule);
		createShaderModule(fragCode, &fragShaderModule);

		VkPipelineShaderStageCreateInfo shaderStages[2];
		// Shader Stage of Vertex
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShaderModule;
		shaderStages[0].pName = "main";
		shaderStages[0].flags = 0;
		shaderStages[0].pNext = nullptr;
		shaderStages[0].pSpecializationInfo = nullptr; // Customization of shader functionality
		// Shader Stage of Fragments
		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShaderModule;
		shaderStages[1].pName = "main";
		shaderStages[1].flags = 0;
		shaderStages[1].pNext = nullptr;
		shaderStages[1].pSpecializationInfo = nullptr; // Customization of shader functionality

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = 0; // Those are inside the shader so we don't provide any informations at this stage
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;

		// Combine viewport and scissor into a single viewport state create info variable
		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; // Struct Type Member
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &configInfo.viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &configInfo.scissor;

		// Will use all of this configuration we just went through to create our actual graphics pipeline object
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2; // How many programmable stages our pipeline will use
		pipelineInfo.pStages = shaderStages; // (for now) we juste have the vertex and fragment shaders stages
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportInfo;
		pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
		pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
		pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = configInfo.pipelineLayout;
		pipelineInfo.renderPass = configInfo.renderPass;
		pipelineInfo.subpass = configInfo.subpass;

		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create graphics pipeline");
		 }

	}

	void Pipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("Fail to run shader module");
		}
	}
}