#include "model.hpp"
#include "utils.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <cassert>
#include <cstring>
#include <unordered_map>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif
#include <iostream>

namespace std {
	template <>
	struct hash<grape::Model::Vertex> {
		size_t operator()(grape::Model::Vertex const& vertex) const {
			size_t seed = 0;
			grape::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace grape {
	// --- Model Class Implementation ---
	Model::Model(Device& device, Builder& builder) : grapeDevice{ device } {
		submeshes = std::move(builder.submeshes);
		texturePaths = std::move(builder.texturePaths);
		materialIdToTexturePath = std::move(builder.materialIdToTexturePath);
		// Get bounding box from builder
		boundingBoxMin = builder.boundingBoxMin;
		boundingBoxMax = builder.boundingBoxMax;
	}

	Model::~Model() {}

	void Model::getBoundingBox(glm::vec3& min, glm::vec3& max) const {
		min = boundingBoxMin;
		max = boundingBoxMax;
	}

	std::unique_ptr<Model> Model::createModelFromFile(Device& device, const std::string& filepath) {
		Builder builder;
		builder.loadModel(device, ENGINE_DIR + filepath);
		return std::make_unique<Model>(device, builder);
	}

	std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

		return attributeDescriptions;
	}

	void Model::drawSubmesh(VkCommandBuffer commandBuffer, uint32_t submeshIndex) {
		assert(submeshIndex < submeshes.size() && "Submesh index out of bounds");
		const auto& submesh = submeshes[submeshIndex];
		if (submesh.indexCount > 0) {
			vkCmdDrawIndexed(commandBuffer, submesh.indexCount, 1, 0, 0, 0);
		}
	}

	void Model::bindSubmesh(VkCommandBuffer commandBuffer, uint32_t submeshIndex) {
		assert(submeshIndex < submeshes.size() && "Submesh index out of bounds");
		const auto& submesh = submeshes[submeshIndex];
		VkBuffer buffers[] = { submesh.vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		if (submesh.indexBuffer) {
			vkCmdBindIndexBuffer(commandBuffer, submesh.indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	// --- Builder Class Implementation ---
    void Model::Builder::loadModel(Device& device, const std::string& filepath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        std::string baseDir = filepath.substr(0, filepath.find_last_of('/') + 1);
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), baseDir.c_str())) {
            throw std::runtime_error(warn + err);
        }

        // Initialize bounding box
        boundingBoxMin = glm::vec3(FLT_MAX);
        boundingBoxMax = glm::vec3(-FLT_MAX);
        bool foundAny = false;

        // Clear previous data
        texturePaths.clear();
        materialIdToTexturePath.clear();

        // Step 1: Create mapping from material ID to texture path
        for (int i = 0; i < materials.size(); ++i) {
            const auto& mat = materials[i];
            if (!mat.diffuse_texname.empty()) {
                materialIdToTexturePath[i] = mat.diffuse_texname;
                // Also store in texturePaths for backwards compatibility
                if (std::find(texturePaths.begin(), texturePaths.end(), mat.diffuse_texname) == texturePaths.end()) {
                    texturePaths.push_back(mat.diffuse_texname);
                }
            }
            else {
                materialIdToTexturePath[i] = ""; // No texture for this material
            }

            std::cout << "Material " << i << " (" << mat.name << "): "
                << (mat.diffuse_texname.empty() ? "No texture" : mat.diffuse_texname) << std::endl;
        }

        // Step 2: Iterate through shapes to build sub-meshes
        for (const auto& shape : shapes) {
            std::map<int, std::unordered_map<Vertex, uint32_t>> materialUniqueVertices;
            std::map<int, std::vector<uint32_t>> materialIndices;

            for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
                const auto& index = shape.mesh.indices[i];
                int material_id = shape.mesh.material_ids[i / 3];

                if (materialUniqueVertices.find(material_id) == materialUniqueVertices.end()) {
                    materialUniqueVertices[material_id] = {};
                    materialIndices[material_id] = {};
                }

                Vertex vertex{};
                if (index.vertex_index >= 0) {
                    vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        -attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2],
                    };

                    // Update bounding box with this vertex position
                    boundingBoxMin = glm::min(boundingBoxMin, vertex.position);
                    boundingBoxMax = glm::max(boundingBoxMax, vertex.position);
                    foundAny = true;

                    // tinyobjloader does not guarantee colors exist, check the size
                    if (attrib.colors.size() > 3 * index.vertex_index) {
                        vertex.color = {
                           attrib.colors[3 * index.vertex_index + 0],
                           attrib.colors[3 * index.vertex_index + 1],
                           attrib.colors[3 * index.vertex_index + 2],
                        };
                    }
                    else {
                        vertex.color = { 1.0f, 1.0f, 1.0f }; // Default white color
                    }
                }
                if (index.normal_index >= 0) {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }
                if (index.texcoord_index >= 0) {
                    vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1],
                    };
                }

                if (materialUniqueVertices.at(material_id).count(vertex) == 0) {
                    materialUniqueVertices.at(material_id)[vertex] = static_cast<uint32_t>(materialUniqueVertices.at(material_id).size());
                }
                materialIndices.at(material_id).push_back(materialUniqueVertices.at(material_id)[vertex]);
            }

            // Step 3: Create sub-meshes and their Vulkan buffers
            for (auto const& [material_id, unique_vertices_map] : materialUniqueVertices) {
                Submesh submesh{};
                submesh.materialId = material_id;
                submesh.indexCount = materialIndices.at(material_id).size();

                std::vector<Vertex> submeshVertices(unique_vertices_map.size());
                for (auto const& [vertex, index] : unique_vertices_map) {
                    submeshVertices[index] = vertex;
                }

                createVertexBuffers(device, submesh.vertexBuffer, submeshVertices);
                createIndexBuffers(device, submesh.indexBuffer, materialIndices.at(material_id));

                // Debug output
                std::string textureName = materialIdToTexturePath.count(material_id) ?
                    materialIdToTexturePath[material_id] : "None";
                std::cout << "Created submesh for material " << material_id
                    << " with texture: " << textureName
                    << " (vertices: " << submeshVertices.size()
                    << ", indices: " << submesh.indexCount << ")" << std::endl;

                submeshes.push_back(std::move(submesh));
            }
        }

        // Handle case where no vertices were found
        if (!foundAny) {
            std::cout << "Warning: No vertices found in model: " << filepath << std::endl;
            boundingBoxMin = glm::vec3(0.0f);
            boundingBoxMax = glm::vec3(0.0f);
        }
        else {
            std::cout << "Model loaded: " << filepath << std::endl;
            std::cout << "  Materials found: " << materials.size() << std::endl;
            std::cout << "  Submeshes created: " << submeshes.size() << std::endl;
            std::cout << "  Bounding box: min(" << boundingBoxMin.x << ", " << boundingBoxMin.y << ", " << boundingBoxMin.z
                << ") max(" << boundingBoxMax.x << ", " << boundingBoxMax.y << ", " << boundingBoxMax.z << ")" << std::endl;
        }
    }

	// --- Builder Helper Functions ---
	void Model::Builder::createVertexBuffers(Device& device, std::unique_ptr<Buffer>& buffer, const std::vector<Vertex>& vertices) {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		uint32_t vertexSize = sizeof(vertices[0]);

		Buffer stagingBuffer{
			device,
			vertexSize,
			static_cast<uint32_t>(vertices.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data());

		buffer = std::make_unique<Buffer>(
			device,
			vertexSize,
			static_cast<uint32_t>(vertices.size()),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		device.copyBuffer(stagingBuffer.getBuffer(), buffer->getBuffer(), bufferSize);
	}

	void Model::Builder::createIndexBuffers(Device& device, std::unique_ptr<Buffer>& buffer, const std::vector<uint32_t>& indices) {
		if (indices.empty()) {
			return;
		}

		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		uint32_t indexSize = sizeof(indices[0]);

		Buffer stagingBuffer{
			device,
			indexSize,
			static_cast<uint32_t>(indices.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		buffer = std::make_unique<Buffer>(
			device,
			indexSize,
			static_cast<uint32_t>(indices.size()),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);
		device.copyBuffer(stagingBuffer.getBuffer(), buffer->getBuffer(), bufferSize);
	}
}