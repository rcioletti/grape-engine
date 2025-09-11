#include "scene_manager.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace grape {
    SceneManager::SceneManager(Device& device, Physics& physics)
        : device(device), physics(physics) {
    }

    void SceneManager::loadScene() {
        loader.loadGameObjects(device, physics, gameObjects);
    }

    void SceneManager::updateScene(float frameTime, GLFWwindow* window) {
        updatePhysics(frameTime);
        handleKinematicMovement(frameTime, window);
    }

    void SceneManager::updatePhysics(float frameTime) {
        physics.StepPhysics(frameTime);

        for (auto& kv : gameObjects) {
            auto& obj = kv.second;
            if (obj.hasPhysics()) {
                obj.updatePhysics();
            }
        }
    }

    void SceneManager::handleKinematicMovement(float frameTime, GLFWwindow* window) {
        for (auto& kv : gameObjects) {
            auto& obj = kv.second;
            if (obj.hasPhysics() && obj.physicsComponent->isKinematic) {
                glm::vec3 movement(0.f);
                if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) movement.z -= 1.f;
                if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) movement.z += 1.f;
                if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) movement.x -= 1.f;
                if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) movement.x += 1.f;

                if (glm::length(movement) > 0.f) {
                    movement = glm::normalize(movement) * frameTime * 3.f;
                    glm::vec3 newPos = obj.transform.translation + movement;
                    obj.setPhysicsTransform(newPos);
                }
                break;
            }
        }
    }
}