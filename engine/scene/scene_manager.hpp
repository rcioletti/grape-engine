#pragma once
#include "game_object.hpp"
#include "systems/physics.hpp"
#include "game_object_loader.hpp"
#include <unordered_map>
#include <memory>

namespace grape {
    class SceneManager {
    public:
        SceneManager(Device& device, Physics& physics);
        ~SceneManager() = default;

        void loadScene();
        void updateScene(float frameTime, GLFWwindow* window);

        std::unordered_map<GameObject::id_t, GameObject>& getGameObjects() { return gameObjects; }
        const std::unordered_map<GameObject::id_t, GameObject>& getGameObjects() const { return gameObjects; }
        const GameObjectLoader& getLoader() { return loader; }


    private:
        void updatePhysics(float frameTime);
        void handleKinematicMovement(float frameTime, GLFWwindow* window);

        std::unordered_map<GameObject::id_t, GameObject> gameObjects;
        GameObjectLoader loader;
        Physics& physics;
        Device& device;
    };
}