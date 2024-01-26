#include "vre_window.hpp"

namespace vre {

	VreWindow::VreWindow(int w, int h, std::string name) : width{ w }, height{h}, windowName {name} {
		initWindow();
	}

	VreWindow::~VreWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VreWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	}
}