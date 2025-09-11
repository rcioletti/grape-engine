#include "camera_controller.hpp"
#include <glm/gtc/constants.hpp>

namespace grape {
    CameraController::CameraController() {
        camera.setViewTarget(glm::vec3(-1.f, 2.f, 2.f), glm::vec3(0.f, 0.f, 0.f));
        viewerObject = std::make_unique<GameObject>(GameObject::createGameObject());
        viewerObject->transform.translation.z = -2.5f;
    }

    void CameraController::update(GLFWwindow* window, float frameTime, float aspectRatio) {
        glm::vec3 cameraPosition = viewerObject->transform.translation;
        glm::vec3 forwardDirection = glm::normalize(viewerObject->transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));

        movementController.moveInPlaneXZ(window, frameTime, *viewerObject);
        camera.setViewDirection(cameraPosition, forwardDirection, glm::vec3(0.0f, -1.0f, 0.0f));
        camera.setPerspectiveProjection(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
    }
}