#pragma once

#include "device.hpp"
#include "renderer/buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>

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
            std::map<int, std::string> materialIdToTexturePath;

            glm::vec3 boundingBoxMin = glm::vec3(0.0f);
            glm::vec3 boundingBoxMax = glm::vec3(0.0f);

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
        const std::map<int, std::string>& getMaterialTextureMapping() const {
            return materialIdToTexturePath;
        }
        size_t getSubmeshCount() const { return submeshes.size(); }

        int getSubmeshMaterialId(uint32_t submeshIndex) const {
            assert(submeshIndex < submeshes.size() && "Submesh index out of bounds");
            return submeshes[submeshIndex].materialId;
        }

        std::string getTexturePathForMaterial(int materialId) const {
            // This would require storing the material-to-texture mapping in the Model class
            // For now, you might need to modify your design slightly
            return "";
        }

        void drawSubmesh(VkCommandBuffer commandBuffer, uint32_t submeshIndex);
        void bindSubmesh(VkCommandBuffer commandBuffer, uint32_t submeshIndex);

        void getBoundingBox(glm::vec3& min, glm::vec3& max) const;

    private:
        Device& grapeDevice;
        std::vector<Submesh> submeshes;
        std::vector<std::string> texturePaths;
        std::map<int, std::string> materialIdToTexturePath;

        glm::vec3 boundingBoxMin = glm::vec3(0.0f);
        glm::vec3 boundingBoxMax = glm::vec3(0.0f);
    };
}