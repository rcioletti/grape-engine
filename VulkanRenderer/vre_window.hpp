#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace vre {
	class VreWindow {
		
	public:
		VreWindow(int w, int h, std::string name);
		~VreWindow();

		VreWindow(const VreWindow&) = delete;
		VreWindow& operator=(const VreWindow&) = delete;

		bool shoudClose() { return glfwWindowShouldClose(window); }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; };

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surf);

	private:
		void initWindow();

		const int width;
		const int height;

		std::string windowName;
		GLFWwindow* window;
	};
}