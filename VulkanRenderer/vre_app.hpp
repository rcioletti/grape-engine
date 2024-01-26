#pragma once

#include "vre_window.hpp"

namespace vre {
	class VreApp {

	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		void run();

	private:
		VreWindow vreWindow{ WIDTH, HEIGHT, "Vulkan 3D Renderer" };
	};
}