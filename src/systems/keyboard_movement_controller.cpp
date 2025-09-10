#include "keyboard_movement_controller.hpp"

#include <iostream>

namespace grape {
    void KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, GameObject& gameObject) {
        glm::vec3 rotate{ 0 };

        // Keyboard rotation
        if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
        if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
        if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
        if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

        // Increase this value to make arrow key rotation faster
        float keyboardLookSpeed = 100.f; // Adjust this value
        rotate *= keyboardLookSpeed;

        // Mouse rotation
        if (glfwGetMouseButton(window, keys.rotateCamera) == GLFW_PRESS) {
            if (!canRotate) {
                glfwGetCursorPos(window, &lastXpos, &lastYpos);
            }
            canRotate = true;

            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            rotate.y -= static_cast<float>(xpos - lastXpos) * lookSpeed;
            rotate.x += static_cast<float>(ypos - lastYpos) * lookSpeed;

            lastXpos = xpos;
            lastYpos = ypos;
        }
        else {
            canRotate = false;
        }

        if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) {
            glm::quat delta_rotation = glm::quat(glm::radians(rotate * dt));
            gameObject.transform.rotation = gameObject.transform.rotation * delta_rotation;
        }

        // Rest of the movement code is the same
        const glm::vec3 rightDir = gameObject.transform.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 upDir = gameObject.transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 fowardDir = gameObject.transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f);

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