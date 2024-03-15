#include "keyboard_movement_controller.hpp"

#include <iostream>

namespace grape {
	void KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject)
	{
		glm::vec3 rotate{ 0 };

		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

		if (glfwGetMouseButton(window, keys.rotateCamera) == GLFW_PRESS) {

			if (!canRotate) {
				glfwGetCursorPos(window, &initialXpos, &initialYpos);
			}

			canRotate = true;

			double xpos, ypos;
			int width, height;
			glfwGetCursorPos(window, &xpos, &ypos);
			glfwGetWindowSize(window, &width, &height);

			if (lastXpos != xpos) {
				rotate.y -= static_cast<uint32_t>(initialXpos - xpos);
				lastXpos = xpos;
			}

			if (lastYpos != ypos) {
				rotate.x += static_cast<uint32_t>(initialYpos - ypos);
				lastYpos = ypos;
			}
		}

		if (glfwGetMouseButton(window, keys.rotateCamera) == GLFW_RELEASE) {
			canRotate = false;
		};

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
		}

		gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
		gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());

		float yaw = gameObject.transform.rotation.y;
		const glm::vec3 fowardDir{ sin(yaw), 0.f, cos(yaw) };
		const glm::vec3 rightDir{ fowardDir.z, 0.f, -fowardDir.x };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		glm::vec3 moveDir{ 0.f };
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += fowardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= fowardDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
			gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		}
	}
}
