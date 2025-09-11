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

	private:
		std::unordered_map<std::string, std::unique_ptr<Texture>> loadedTextures;
		std::unordered_map<std::string, int> texturePathToDescriptorIndex;

		void createTexturePathToIndexMapping(GameObject::Map &gameObjects);
	};
}