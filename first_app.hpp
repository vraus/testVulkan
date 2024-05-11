#pragma once

#include "window.hpp"
#include "pipeline.hpp"
#include "swapChain.hpp"
#include "device.hpp"

#include <memory>
#include <vector>

namespace vraus_VulkanEngine {
	class FirstApp {
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp& operator=(const FirstApp&) = delete;

		void run();
	private:
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void drawFrame();

		Window window{ WIDTH, HEIGHT, "Vulkan App" };
		Device device{ window };
		SwapChain swapChain{ device, window.getExtend()};
		std::unique_ptr<Pipeline> pipeline; // Smart pointer
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
	};
}