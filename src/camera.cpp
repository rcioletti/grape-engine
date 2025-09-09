#include "camera.hpp"

#include <cassert>
#include <limits>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace grape {

    void Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
        // Vulkan's NDC has a Z range of [0, 1] and Y-down.
        // glm::ortho doesn't have a direct Y-down option, so we flip it manually.
        projectionMatrix = glm::ortho(left, right, bottom, top, near, far);
        projectionMatrix[1][1] *= -1.0f;
    }

    void Camera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
        // Use GLM's standard perspective function for a right-handed, Z-to-[0,1] range
        projectionMatrix = glm::perspectiveRH_ZO(fovy, aspect, near, far);

        // Now, manually flip the Y-axis to match Vulkan's Y-down convention
        projectionMatrix[1][1] *= -1.0f;
    }

    void Camera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
        // Use glm::lookAt for a reliable view matrix
        viewMatrix = glm::lookAt(position, position + direction, up);
        inverseViewMatrix = glm::inverse(viewMatrix);
    }

    void Camera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        // Use glm::lookAt for a reliable view matrix
        viewMatrix = glm::lookAt(position, target, up);
        inverseViewMatrix = glm::inverse(viewMatrix);
    }

    void Camera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        // This calculates the camera's rotation matrix from Euler angles
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, rotation.y, glm::vec3(0.f, 1.f, 0.f));
        rotationMatrix = glm::rotate(rotationMatrix, rotation.x, glm::vec3(1.f, 0.f, 0.f));
        rotationMatrix = glm::rotate(rotationMatrix, rotation.z, glm::vec3(0.f, 0.f, 1.f));

        // View matrix is the inverse of the camera's world transform
        viewMatrix = glm::inverse(glm::translate(glm::mat4(1.0f), position) * rotationMatrix);
        inverseViewMatrix = glm::translate(glm::mat4(1.0f), position) * rotationMatrix;
    }
}