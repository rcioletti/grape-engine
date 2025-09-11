#pragma once
#include "renderer/camera.hpp"
#include "game_object.hpp"
#include "systems/keyboard_movement_controller.hpp"
#include <GLFW/glfw3.h>

namespace grape {
    class CameraController {
    public:
        CameraController();
        ~CameraController() = default;

        void update(GLFWwindow* window, float frameTime, float aspectRatio);
        Camera& getCamera() { return camera; }
        const Camera& getCamera() const { return camera; }

    private:
        Camera camera;
        std::unique_ptr<GameObject> viewerObject;
        KeyboardMovementController movementController;
    };
}