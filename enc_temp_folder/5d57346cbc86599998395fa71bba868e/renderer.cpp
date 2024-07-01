#include "renderer.hpp"

#include <stdexcept>
#include <array>

namespace vraus_VulkanEngine {

	Renderer::Renderer(Window& _window, Device& _device) : window{ _window }, device{ _device } {
		recreateSwapChain();
		createCommandBuffers();
	}

	Renderer::~Renderer() { freeCommandBuffers(); } // It is possible that the renderer will be destroyed but not the application

	VkCommandBuffer Renderer::beginFrame()
	{
		assert(!isFrameStarted && "Can't call beginFrame while already in progress.");

		auto result = swapChain->acquireNextImage(&currentImageIndex);

		// VK_ERROR_OUT_OF_DATE_KHR: A surface has changed in such a way that is is no longer compatible with the swapchain,
		// and further presentation requests using the swapchain will fail. Applications MUST query the new surface properties
		// and recreate their swapchain if they wish to continue presenting to the surface.
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr; // force stop the current out of date presentation
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		return commandBuffer;
	}

	void Renderer::endFrame()
	{
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress.");
		auto commandBuffer = getCurrentCommandBuffer();

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to record command buffer");
		}

		// Submit the provided command buffer to the device graphics queue, while handling CPU & GPU synchronization
		// The command buffer will then be executed
		// The swap chain will present the associated color attachment image view to the display at the appropriate time, based on the present mode selected
		auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		// VK_SUBOPTIMAL_KHR: A swapchain no longer matches the surface properties exactly
		// but CAN still be used to present to the surface successfully.
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
			window.resetWindowResizedFlag();
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to present swap chain image");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(isFrameStarted && "Cannot beginSwapChainRenderPass while frame is not in progress.");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't beging render pass on a command buffer from a different frame.");
	
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = swapChain->getRenderPass();
		renderPassInfo.framebuffer = swapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 }; // define the area where the shader loads and stores will take place
		renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent(); // For high density displays, the swap chain extent may be larger than our window's

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// VK_SUBPASS_CONTENTS_INLINE : Signals that the subsequent render pass commands will be directly embeded in the primary command buffer itself, no secondary command buffer will be used
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(swapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, swapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(isFrameStarted && "Cannot endSwapChainRenderPass while frame is not in progress.");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on a command buffer from a different frame.");
		
		vkCmdEndRenderPass(commandBuffer);
	}

	void Renderer::createCommandBuffers()
	{
		// The max frames in flight constant is currently defined as 2
		commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary command buffer can be submited to a queue for submission but cannot be called by other command buffers. SECONDARY are exact opposit
		allocInfo.commandPool = device.getCommandPool(); // Opaque objects that command buffer memory are allocated from
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate command buffers");
		}
	}

	void Renderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	void Renderer::recreateSwapChain()
	{
		auto extent = window.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = window.getExtent();
			glfwWaitEvents(); // While there is at least one dimmension with no size the programme will wait (i.e: minimization)
		}

		vkDeviceWaitIdle(device.device()); // Wait until the current swap chain is no longer being used before we create the new swap chain

		if (swapChain == nullptr) {
			swapChain = std::make_unique<SwapChain>(device, extent);
		}
		else {
			std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
			swapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*swapChain.get())) {
				throw std::runtime_error("Swap chain image(or depth) format has changed.");
				// Create a callback here later to handle this case.
			}
		}

	}
}