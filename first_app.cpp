#include "first_app.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <stdexcept>
#include <array>

namespace vraus_VulkanEngine {

	struct SimplePushConstantData {
		glm::mat2 transform{ 1.f };
		glm::vec2 offset;
		alignas (16) glm::vec3 color;
	};

	FirstApp::FirstApp() {
		loadGameObjects();
		createPipelineLayout();
		createPipeline();
	}

	FirstApp::~FirstApp() {
		vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
	}

	void FirstApp::run() {
		while (!window.shouldClose()) {
			glfwPollEvents();

			if (auto commandBuffer = renderer.beginFrame()) { // beginFrame function returns null if the swapChain needs to be recreated
				
				// Example of usage and why we put every steps of drawing a frame appart:
				// Begin offscreen shadow pass
				// Render shadow casting objects
				// End offscreen shadow pass
				
				renderer.beginSwapChainRenderPass(commandBuffer);
				renderGameObjects(commandBuffer);
				renderer.endSwapChainRenderPass(commandBuffer);
				renderer.endFrame();
			}
		}
		vkDeviceWaitIdle(device.device()); // To block the CPU until all GPU operations are completed. We can then safely clean up all resources.
	}

	void FirstApp::loadGameObjects()
	{
		std::vector<Model::Vertex> vertices{
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		auto model = std::make_shared<Model>(device, vertices); // Allows to have ONE model instance used by MULTIPLE game objects
		
		// https://www.color-hex.com/color-palette/5361
		std::vector<glm::vec3> colors{
		  {1.f, .7f, .73f},
		  {1.f, .87f, .73f},
		  {1.f, 1.f, .73f},
		  {.73f, 1.f, .8f},
		  {.73, .88f, 1.f}
		};
		for (auto& color : colors) {
			color = glm::pow(color, glm::vec3{ 2.2f });
		}
		for (int i = 0; i < 40; i++) {
			auto triangle = GameObject::createGameObject();
			triangle.model = model;
			triangle.transform2d.scale = glm::vec2(.5f) + i * 0.025f;
			triangle.transform2d.rotation = i * glm::pi<float>() * .025f;
			triangle.color = colors[i % colors.size()];
			gameObjects.push_back(std::move(triangle));
		}
		
	}

	void FirstApp::createPipelineLayout()
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr; // Pass data (other than vertex data) to our vertex and fragment shaders (texture, uniform buffer objects, etc)
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Way to very efficiently send a small amount of data to shader programs

		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout");
		}

	}

	void FirstApp::createPipeline()
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		Pipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderer.getSwapChainRenderPass(); // Render pass describes the sctructure and format of our frame buffer object and their attachments
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipeline = std::make_unique<Pipeline>(
			device,
			"simple_shader.vert.spv",
			"simple_shader.frag.spv",
			pipelineConfig);
	}
	
	void FirstApp::renderGameObjects(VkCommandBuffer commandBuffer)
	{
		int i = 0;
		for (auto& obj : gameObjects) {
			i += 1;
			obj.transform2d.rotation = glm::mod<float>(obj.transform2d.rotation + 0.00001f * i, 2.f * glm::pi<float>());
		}

		pipeline->bind(commandBuffer);

		for (auto& obj : gameObjects) {
			obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.0001f, glm::two_pi<float>()); // This will rotate the triangle in a full circle

			SimplePushConstantData push{};
			push.offset = obj.transform2d.translation;
			push.color = obj.color;
			push.transform = obj.transform2d.mat2();

			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SimplePushConstantData), &push);
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}
}