#pragma once

#include "vre_window.hpp"
#include "vre_device.hpp"
#include "vre_game_object.hpp"
#include "vre_renderer.hpp"
#include "vre_descriptors.hpp"

#include <memory>
#include <vector>

namespace vre {
	class VreApp {

	public:
		static constexpr int WIDTH = 1280;
		static constexpr int HEIGHT = 720;

		VreApp();
		~VreApp();

		VreApp(const VreApp&) = delete;
		VreApp& operator=(const VreApp&) = delete;

		void run();

	private:
		void loadGameObjects();

		VreWindow vreWindow{ WIDTH, HEIGHT, "3D Game Engine" };
		VreDevice vreDevice{ vreWindow };
		VreRenderer vreRenderer{ vreWindow, vreDevice };

		std::unique_ptr<VreDescriptorPool> globalPool{};
		std::unique_ptr<VreDescriptorPool> imagePool{};
		VreGameObject::Map gameObjects;
	};
}