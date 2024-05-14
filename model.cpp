#include "model.hpp"

#include <cassert>
#include <cstring>

namespace vraus_VulkanEngine {

	Model::Model(Device& device, const std::vector<Vertex>& vertices) : device{ device } {
		createVertexBuffers(vertices);
	}

	Model::~Model()
	{
		vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
		vkFreeMemory(device.device(), vertexBufferMemory, nullptr);
	}

	/* Record to our command buffer to bind 1 vertex buffers starting at biding 0
	with an offset of 0 into the buffer. We can add additionnal elements
	by adding them to the buffers and offsets arrays into the Model::bind() function.*/
	void Model::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	}

	void Model::draw(VkCommandBuffer commandBuffer)
	{
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}

	void Model::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount; // Total number of bits required for our vertex buffer to store all the vertices of our model
		// HOST: CPU, Device : GPU
		// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: Tells Vulkan that we want that allocated can be accessible from our host. Necessary so that the host can write on our device memory.
		// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Keeps the host and device memory regions consistent with each other, changes are then propagated in one an other.
		device.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffer,
			vertexBufferMemory
		);
		void* data;
		// Creates a region of host memory, maped to device memory and sets data to point to the begining of the maped memory range
		vkMapMemory(device.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
		// Takes the vertices data and copies it into the host maped memory region
		// Because of the coherence bit property, the host memory will automaticly be flushed to update the device memory. Otherwise use Flushed() to propagate changes
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(device.device(), vertexBufferMemory);
	}

	/* This binding description correspond to our single vertex buffer. 
	It will occupy the first biding at index 0, the stride advances by size of Vertex byte per vertex. */
	std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
		// or with brace construction: 
		// return { {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX} };
	}

	std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0; // Location specified in the vertex shader
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // 2 elements of type float, vec2 float
		attributeDescriptions[0].offset = offsetof(Vertex, position);
		
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1; // Location specified in the vertex shader
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // 2 elements of type float, vec2 float
		attributeDescriptions[1].offset = offsetof(Vertex, color); // calculate the offset of the color member in the vertex struct

		return attributeDescriptions;
		// or with brace construction:
		// return { {0, 0, VK_FORMAT_R32G32_SFLOAT, 0} };
	}

}
