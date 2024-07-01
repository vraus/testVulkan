#pragma once

#include "window.hpp"
#include "swapChain.hpp"
#include "device.hpp"
#include "model.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace vraus_VulkanEngine {
	class Renderer {
	public:
		Renderer(Window& window, Device& device);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return swapChain->getRenderPass(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command Buffer when frame not in progress");
			return commandBuffers[currentImageIndex]; 
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress.");
			return currentFrameIndex;
		}

		// We want the application to main control over every steps of drawing a frame
		// so that down the line we can easily integrate multiple render passes
		// Will be helpfull for things like reflections, shadows, raytracing, postprocessing effects

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		Window& window;
		Device& device;
		std::unique_ptr<SwapChain> swapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{ 0 }; // Keep track of a frameIndex : [0, Max_Frames_In_Flight] not tight to the image index.
		bool isFrameStarted{ false };
	};
}