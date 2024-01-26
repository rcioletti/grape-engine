#include "vre_app.hpp"

namespace vre {

	void VreApp::run()
	{
		while (!vreWindow.shoudClose()) {
			glfwPollEvents();
		}
	}
}