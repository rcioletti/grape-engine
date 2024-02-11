#include "vre_model.hpp"

#include <cassert>
#include <cstring>

namespace vre {

	VreModel::VreModel(VreDevice& device, const VreModel::Builder &builder) : vreDevice{device}
	{
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	VreModel::~VreModel()
	{
		vkDestroyBuffer(vreDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(vreDevice.device(), vertexBufferMemory, nullptr);

		if (hasIndexBuffer) {

			vkDestroyBuffer(vreDevice.device(), indexBuffer, nullptr);
			vkFreeMemory(vreDevice.device(), indexBufferMemory, nullptr);
		}
	}

	void VreModel::draw(VkCommandBuffer commandBuffer)
	{
		if (hasIndexBuffer) {

			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else {

			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	void VreModel::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void VreModel::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		vreDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(vreDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(vreDevice.device(), stagingBufferMemory);

		vreDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);

		vreDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(vreDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(vreDevice.device(), stagingBufferMemory, nullptr);
	}

	void VreModel::createIndexBuffers(const std::vector<uint32_t>& indices)
	{
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;
		
		if (!hasIndexBuffer) {
			return;
		}

		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		vreDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(vreDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(vreDevice.device(), stagingBufferMemory);

		vreDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			indexBuffer,
			indexBufferMemory);

		vreDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(vreDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(vreDevice.device(), stagingBufferMemory, nullptr);
	}

	std::vector<VkVertexInputBindingDescription> VreModel::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> VreModel::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);


		return attributeDescriptions;
	}
}
