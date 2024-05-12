#pragma once

#include "device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm.hpp>

#include <vector>

namespace vraus_VulkanEngine {
	/* The purpose of this class is to be able to take vertex data created by or read in a file on the cpu, 
	and then allocate the memory and copy the data over to our device GPU so it can be rendered efficiently.
	*/
	class Model {
	public:

		struct Vertex {
			glm::vec2 position;
			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		Model(Device &device, const std::vector<Vertex> &vertices);
		~Model();

		// We must delete the copy constructor because model class manages the vulkan buffer and memory object
		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private: 

		void createVertexBuffers(const std::vector<Vertex>& vertices);

		Device& device;
		VkBuffer vertexBuffer; // Buffer and its assigned memory are two seperate objects
		VkDeviceMemory vertexBufferMemory;
		uint32_t vertexCount;
	};
}