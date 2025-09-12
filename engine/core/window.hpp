#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace grape {
	class Window {
		
	public:
		Window(int w, int h, std::string name);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		bool shoudClose() { return glfwWindowShouldClose(window); }
		VkExtent2D getExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		bool wasWindowResized() { return frameBufferResized; }
		void resetWindowResizedFlag() { frameBufferResized = false; }
		GLFWwindow* getGLFWwindow() const { return window; }

		void createWindowSurface(VkInstance instance, VkSurfaceKHR* surf);

	private:
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
		void setWindowIcon(const char* iconPath);
		void initWindow();

		int width;
		int height;
		bool frameBufferResized = false;

		std::string windowName;
		GLFWwindow* window;
	};
}