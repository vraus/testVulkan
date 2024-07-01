#pragma once

#include "pipeline.hpp"
#include "device.hpp"
#include "model.hpp"
#include "game_object.hpp"

#include <memory>
#include <vector>

namespace vraus_VulkanEngine {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(Device& device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass); // The render pass is used specifically to create the pipeline

		Device& device;

		std::unique_ptr<Pipeline> pipeline; // Smart pointer
		VkPipelineLayout pipelineLayout;
	};
}