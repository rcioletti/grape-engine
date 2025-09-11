#pragma once

#include "game_object_loader.hpp"
#include "game_object.hpp"

#include "renderer/model.hpp"
#include "renderer/texture.hpp"

#include <vector>

namespace grape {
	class GameObjectLoader {

	public:

		GameObjectLoader();
		~GameObjectLoader();

		void loadGameObjects(Device& grapeDevice, Physics physics, GameObject::Map& gameObjects);

		// Optional: getter for loaded textures (might be useful for debugging)
		const std::unordered_map<std::string, std::unique_ptr<Texture>>& getLoadedTextures() const {
			return loadedTextures;
		}

		// Optional: getter for texture mapping (for debugging)
		const std::unordered_map<std::string, int>& getTextureMapping() const {
			return texturePathToDescriptorIndex;
		}

		int getTextureDescriptorIndex(const std::string& texturePath) const;

        std::vector<std::string> getOrderedTexturePaths() const {
            return orderedTexturePaths;
        }

        // Get texture at specific descriptor index
        const Texture* getTextureAtIndex(int index) const {
            if (index == 0) {
                // Always return fallback texture for index 0
                return fallbackTexture.get();
            }

            if (index - 1 < orderedTexturePaths.size()) {
                const std::string& path = orderedTexturePaths[index - 1];
                auto it = loadedTextures.find(path);
                if (it != loadedTextures.end()) {
                    return it->second.get();
                }
            }

            // Return fallback if not found
            return fallbackTexture.get();
        }

	private:
		std::unordered_map<std::string, std::unique_ptr<Texture>> loadedTextures;
		std::unordered_map<std::string, int> texturePathToDescriptorIndex;

		void createTexturePathToIndexMapping(GameObject::Map &gameObjects);

        std::vector<std::string> orderedTexturePaths;  // Keep track of texture order
        mutable std::unique_ptr<Texture> fallbackTexture;  // Fallback texture

        // Helper method to collect unique texture paths
        std::vector<std::string> collectUniqueTexturePaths(const GameObject::Map& gameObjects);
	};
}