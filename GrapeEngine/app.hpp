#pragma once

#include "window.hpp"
#include "device.hpp"
#include "game_object.hpp"
#include "renderer.hpp"
#include "descriptors.hpp"

#include <memory>
#include <vector>

namespace grape {
	class App {

	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;

		App();
		~App();

		App(const App&) = delete;
		App& operator=(const App&) = delete;

		void run();

	private:
		void loadGameObjects();

		Window grapeWindow{ WIDTH, HEIGHT, "Grape Engine" };
		Device grapeDevice{ grapeWindow };
		Renderer grapeRenderer{ grapeWindow, grapeDevice };

		std::unique_ptr<DescriptorPool> globalPool{};
		std::unique_ptr<DescriptorPool> imagePool{};
		GameObject::Map gameObjects;

		std::vector<Texture> textures;
	};
}