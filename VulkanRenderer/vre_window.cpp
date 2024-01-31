#include "vre_window.hpp"

#include <stdexcept>

namespace vre {

	VreWindow::VreWindow(int w, int h, std::string name) : width{ w }, height{h}, windowName {name} {
		initWindow();
	}

	VreWindow::~VreWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VreWindow::framebufferResizeCallback(GLFWwindow* window, int width, int height)
	{
		auto vreWindow = reinterpret_cast<VreWindow*>(glfwGetWindowUserPointer(window));
		vreWindow->frameBufferResized = true;
		vreWindow->width = width;
		vreWindow-> height = height;
	}

	void VreWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void VreWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}
}