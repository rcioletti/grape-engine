#include "window.hpp"

#include <stdexcept>

namespace grape {

	Window::Window(int w, int h, std::string name) : width{ w }, height{h}, windowName {name} {
		initWindow();
	}

	Window::~Window() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto grapeWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		grapeWindow->frameBufferResized = true;
		grapeWindow->width = width;
		grapeWindow-> height = height;
	}

	void Window::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}
}