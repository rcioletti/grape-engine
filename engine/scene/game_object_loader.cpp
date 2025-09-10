#include "game_object_loader.hpp"
#include "renderer/model.hpp"
#include "renderer/texture.hpp"
#include "game_object.hpp"

#include <memory>
#include <iostream>

namespace grape {

	GameObjectLoader::GameObjectLoader()
	{
	}

	GameObjectLoader::~GameObjectLoader()
	{
		// Cleanup textures from the map
		loadedTextures.clear(); // This calls the destructors for all unique_ptr<Texture> objects
	}

	void GameObjectLoader::loadGameObjects(Device& grapeDevice, Physics physics, GameObject::Map& gameObjects)
	{
		// Load the arcade model
		std::shared_ptr<Model> arcadeModel = Model::createModelFromFile(grapeDevice, "resources/models/Asteroids.obj");

		// Get the material-to-texture mapping from the builder
		const auto& modelTexturePaths = arcadeModel->getTexturePaths();

		std::cout << "Loading textures for arcade model:" << std::endl;
		for (const auto& path : modelTexturePaths) {
			if (!path.empty() && loadedTextures.find(path) == loadedTextures.end()) {
				std::cout << "  Loading texture: " << path << std::endl;
				try {
					auto newTexture = std::make_unique<Texture>(grapeDevice);
					newTexture->createTextureFromFile("resources/textures/" + path);
					loadedTextures.emplace(path, std::move(newTexture));
					std::cout << "    Success!" << std::endl;
				}
				catch (const std::exception& e) {
					std::cout << "    Failed to load texture " << path << ": " << e.what() << std::endl;
				}
			}
			else if (!path.empty()) {
				std::cout << "  Texture already loaded: " << path << std::endl;
			}
		}

		// Create the arcade game object
		auto arcade = GameObject::createPhysicsObject(physics, glm::vec3(0.f, -5.f, 0.f), true, false);
		arcade.name = "Arcade";
		arcade.model = arcadeModel;
		arcade.transform.scale = glm::vec3(1.f);
		arcade.transform.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(1.f, 0.f, 0.f));

		// Compute bounding box
		glm::vec3 min, max;
		arcadeModel->getBoundingBox(min, max);
		glm::vec3 size = max - min;
		glm::vec3 halfExtents = 0.5f * size * arcade.transform.scale;

		std::cout << "Arcade model bounding box:" << std::endl;
		std::cout << "  Min: (" << min.x << ", " << min.y << ", " << min.z << ")" << std::endl;
		std::cout << "  Max: (" << max.x << ", " << max.y << ", " << max.z << ")" << std::endl;
		std::cout << "  Half extents: (" << halfExtents.x << ", " << halfExtents.y << ", " << halfExtents.z << ")" << std::endl;
		std::cout << "  Submesh count: " << arcadeModel->getSubmeshCount() << std::endl;

		// Add collider with correct size
		arcade.addBoxCollider(physics, halfExtents);
		gameObjects.emplace(arcade.getId(), std::move(arcade));

		// Create point lights
		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = GameObject::makePointLight(1.2f);
			pointLight.color = lightColors[i];

			// Calculate angle for this light in the circle
			float angle = (i * glm::two_pi<float>()) / lightColors.size();

			// Position lights in a circle ABOVE the floor
			float radius = 2.0f;  // Distance from center
			float height = -2.0f;  // Height above floor (positive Y)

			pointLight.transform.translation = glm::vec3(
				radius * cos(angle),  // X position (circle)
				height,               // Y position (above floor)
				radius * sin(angle)   // Z position (circle)
			);

			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
		// Load the plane model for floor
		std::shared_ptr<Model> planeModel = Model::createModelFromFile(grapeDevice, "resources/models/plane.obj");

		// Load textures for plane
		const auto& planeTexturePaths = planeModel->getTexturePaths();
		for (const auto& path : planeTexturePaths) {
			if (!path.empty() && loadedTextures.find(path) == loadedTextures.end()) {
				try {
					auto newTexture = std::make_unique<Texture>(grapeDevice);
					newTexture->createTextureFromFile("resources/textures/" + path);
					loadedTextures.emplace(path, std::move(newTexture));
				}
				catch (const std::exception& e) {
					std::cout << "Failed to load plane texture " << path << ": " << e.what() << std::endl;
				}
			}
		}

		// Create floor game object
		auto floor = GameObject::createGameObject();
		floor.name = "Floor";
		floor.model = planeModel;
		floor.transform.translation = glm::vec3(0.f, 1.f, 0.f);
		floor.transform.scale = glm::vec3(10.f, 1.f, 10.f);
		gameObjects.emplace(floor.getId(), std::move(floor));

		// IMPORTANT: Create the texture mapping after all textures are loaded
		createTexturePathToIndexMapping(gameObjects);
	}

	void GameObjectLoader::createTexturePathToIndexMapping(GameObject::Map &gameObjects) {
		texturePathToDescriptorIndex.clear();

		// Collect all unique texture paths from all game objects (same logic as in run())
		std::vector<std::string> allUniqueTexturePaths;
		for (auto const& [id, obj] : gameObjects) {
			if (obj.model) {
				const auto& modelPaths = obj.model->getTexturePaths();
				for (const auto& path : modelPaths) {
					if (!path.empty()) {
						bool found = false;
						for (const auto& uniquePath : allUniqueTexturePaths) {
							if (uniquePath == path) {
								found = true;
								break;
							}
						}
						if (!found) {
							allUniqueTexturePaths.push_back(path);
						}
					}
				}
			}
		}

		std::cout << "Creating texture path to descriptor index mapping:" << std::endl;

		// Map texture paths to their indices (starting from index 1, since 0 is fallback)
		for (size_t i = 0; i < allUniqueTexturePaths.size(); i++) {
			const std::string& path = allUniqueTexturePaths[i];
			int descriptorIndex = static_cast<int>(i + 1); // +1 because index 0 is fallback

			texturePathToDescriptorIndex[path] = descriptorIndex;
			std::cout << "  '" << path << "' -> descriptor index " << descriptorIndex << std::endl;
		}

		std::cout << "Total texture mappings created: " << texturePathToDescriptorIndex.size() << std::endl;
	}

	// Getter for texture descriptor index
	int GameObjectLoader::getTextureDescriptorIndex(const std::string& texturePath) {
		if (texturePath.empty()) return 0;

		auto it = texturePathToDescriptorIndex.find(texturePath);
		if (it != texturePathToDescriptorIndex.end()) {
			return it->second;
		}
		return 0; // Fallback to default texture
	}
}