#include "first_app.hpp"

#include <stdexcept>
#include <array>

namespace vraus_VulkanEngine {

	FirstApp::FirstApp() {
		loadModels();
		createPipelineLayout();
		recreateSwapChain();
		createCommandBuffers();
	}

	FirstApp::~FirstApp() {
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	void FirstApp::run() {
		while (!window.shouldClose()) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device.device()); // To block the CPU until all GPU operations are completed. We can then safely clean up all resources.
	}

	void FirstApp::loadModels()
	{
		std::vector<Model::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};

		model = std::make_unique<Model>(device, vertices);
	}

	void FirstApp::createPipelineLayout()
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr; // Pass data (other than vertex data) to our vertex and fragment shaders (texture, uniform buffer objects, etc)
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Way to very efficiently send a small amount of data to shader programs

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout");
		}

	}

	void FirstApp::createPipeline()
	{
		// It's important to use the swapChain width and height as it doesn't necessarly match the window's
		auto pipelineConfig = Pipeline::defaultPipelineConfigInfo(swapChain->width(), swapChain->height());
		pipelineConfig.renderPass = swapChain->getRenderPass(); // Render pass describes the sctructure and format of our frame buffer object and their attachments
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(
			device,
			"simple_shader.vert.spv",
			"simple_shader.frag.spv",
			pipelineConfig
		);
	}

	void FirstApp::createCommandBuffers()
	{
		commandBuffers.resize(swapChain->imageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary command buffer can be submited to a queue for submission but cannot be called by other command buffers. SECONDARY are exact opposit
		allocInfo.commandPool = device.getCommandPool(); // Opaque objects that command buffer memory are allocated from
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers");
		}
	}

	void FirstApp::drawFrame()
	{
		uint32_t imageIndex;
		auto result = swapChain->acquireNextImage(&imageIndex);

		// VK_ERROR_OUT_OF_DATE_KHR: A surface has changed in such a way that is is no longer compatible with the swapchain,
		// and further presentation requests using the swapchain will fail. Applications MUST query the new surface properties
		// and recreate their swapchain if they wish to continue presenting to the surface.
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return; // force stop the current out of date presentation
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}

		recordCommandBuffer(imageIndex);

		// Submit the provided command buffer to the device graphics queue, while handling CPU & GPU synchronization
		// The command buffer will then be executed
		// The swap chain will present the associated color attachment image view to the display at the appropriate time, based on the present mode selected
		result = swapChain->submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
		// VK_SUBOPTIMAL_KHR: A swapchain no longer matches the surface properties exactly
		// but CAN still be used to present to the surface successfully.
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
			window.resetWindowResizedFlag();
			recreateSwapChain();
			return;
		}
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image");
		}
	}

	void FirstApp::recreateSwapChain()
	{
		auto extent = window.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = window.getExtent();
			glfwWaitEvents(); // While there is at least one dimmension with no size the programme will wait (i.e: minimization)
		}

		vkDeviceWaitIdle(device.device()); // Wait until the current swap chain is no longer being used before we create the new swap chain
		swapChain = nullptr;
		swapChain = std::make_unique<SwapChain>(device, extent);
		createPipeline(); // Pipeline depends on the swap chain so we need to recreate the pipeline following swap chain creation
	}

	void FirstApp::recordCommandBuffer(int _imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffers[_imageIndex], &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapChain->getRenderPass();
		renderPassInfo.framebuffer = swapChain->getFrameBuffer(_imageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 }; // define the area where the shader loads and stores will take place
		renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent(); // For high density displays, the swap chain extent may be larger than our window's

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// VK_SUBPASS_CONTENTS_INLINE : Signals that the subsequent render pass commands will be directly embeded in the primary command buffer itself, no secondary command buffer will be used
		vkCmdBeginRenderPass(commandBuffers[_imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		pipeline->bind(commandBuffers[_imageIndex]);
		model->bind(commandBuffers[_imageIndex]);
		model->draw(commandBuffers[_imageIndex]);

		vkCmdEndRenderPass(commandBuffers[_imageIndex]);
		if (vkEndCommandBuffer(commandBuffers[_imageIndex]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}
	}
}