#pragma once

#include "device.hpp"
#include "buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>

namespace grape {
    class Model {
    public:
        struct Vertex {
            glm::vec3 position{};
            glm::vec3 color{};
            glm::vec3 normal{};
            glm::vec2 uv{};

            static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

            bool operator==(const Vertex& other) const {
                return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
            }
        };

        struct Submesh {
            std::unique_ptr<Buffer> vertexBuffer;
            std::unique_ptr<Buffer> indexBuffer;
            uint32_t indexCount;
            int materialId; // Changed to int to match tinyobjloader's material_id
        };

        class Builder {
        public:
            void loadModel(Device& device, const std::string& filepath);
            std::vector<Submesh> submeshes;
            std::vector<std::string> texturePaths;

        private:
            void createVertexBuffers(Device& device, std::unique_ptr<Buffer>& buffer, const std::vector<Vertex>& vertices);
            void createIndexBuffers(Device& device, std::unique_ptr<Buffer>& buffer, const std::vector<uint32_t>& indices);
        };

        Model(Device& device, Builder& builder);
        ~Model();

        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;

        static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filepath);

        // Public accessors for sub-meshes and texture paths
        const std::vector<Submesh>& getSubmeshes() const { return submeshes; }
        const std::vector<std::string>& getTexturePaths() const { return texturePaths; }

        void drawSubmesh(VkCommandBuffer commandBuffer, uint32_t submeshIndex);
        void bindSubmesh(VkCommandBuffer commandBuffer, uint32_t submeshIndex);

    private:
        Device& grapeDevice;
        std::vector<Submesh> submeshes;
        std::vector<std::string> texturePaths;
    };
}