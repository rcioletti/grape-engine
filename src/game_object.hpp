// Enhanced game_object.hpp with physics integration
#pragma once
#include "model.hpp"
#include "physics.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <unordered_map>

namespace grape {
    struct TransformComponent {
        glm::vec3 translation{};
        glm::vec3 scale{ 1.f, 1.f, 1.f };
        glm::quat rotation{ 1.f, 0.f, 0.f, 0.f }; // identity

        glm::mat4 mat4() const {
            glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 R = glm::toMat4(rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
            return T * R * S;
        }

        glm::mat3 normalMatrix() const {
            glm::mat4 model = mat4();
            return glm::mat3(glm::transpose(glm::inverse(model)));
        }

        PxTransform toPxTransform() const {
            // Convert from Y-down (your system) to Y-up (PhysX)
            return PxTransform(
                PxVec3(translation.x, -translation.y, translation.z),
                PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)
            );
        }

        // Update transform from PhysX actor
        void updateFromPhysX(PxRigidActor* actor) {
            if (!actor) return;

            PxTransform pxTransform = actor->getGlobalPose();
            translation = glm::vec3(pxTransform.p.x, pxTransform.p.y, pxTransform.p.z);
            rotation = glm::quat(pxTransform.q.w, pxTransform.q.x, pxTransform.q.y, pxTransform.q.z);
        }

        // Convenience helpers
        void setEulerRadians(const glm::vec3& eulerRad) {
            rotation = glm::quat(eulerRad);
        }
        void setEulerDegrees(const glm::vec3& eulerDeg) {
            setEulerRadians(glm::radians(eulerDeg));
        }
        glm::vec3 getEulerRadians() const {
            return glm::eulerAngles(rotation);
        }
        glm::vec3 getEulerDegrees() const {
            return glm::degrees(getEulerRadians());
        }
    };

    struct PointLightComponent {
        float lightIntensity = 1.0f;
    };

    // New Physics Component
    struct PhysicsComponent {
        PxRigidActor* actor = nullptr;
        PxShape* shape = nullptr;
        bool isKinematic = false;
        bool isDynamic = true;
        float mass = 1.0f;

        // Physics material properties
        float staticFriction = 0.5f;
        float dynamicFriction = 0.5f;
        float restitution = 0.6f;

        ~PhysicsComponent() {
            // Note: PhysX objects should be released by the Physics system
            // Don't release them here to avoid double-deletion
        }
    };

    class GameObject {
    public:
        using id_t = unsigned int;
        using Map = std::unordered_map<id_t, GameObject>;

        static GameObject createGameObject() {
            static id_t currentId = 0;
            return GameObject{ currentId++ };
        }

        static GameObject makePointLight(float intensity = 10.f, float radius = 0.1f, glm::vec3 color = glm::vec3(1.f));

        // Physics-enabled factory methods
        static GameObject createPhysicsObject(Physics& physics, const glm::vec3& position = glm::vec3(0.f),
            bool isDynamic = true, bool isKinematic = false) {
            auto obj = createGameObject();
            obj.transform.translation = position;
            obj.addPhysicsComponent(physics, isDynamic, isKinematic);
            return obj;
        }

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        GameObject& operator=(GameObject&&) = default;

        id_t getId() { return id; }

        // Physics methods
        void addPhysicsComponent(Physics& physics, bool isDynamic = true, bool isKinematic = false) {
            if (physicsComponent) return; // Already has physics

            physicsComponent = std::make_unique<PhysicsComponent>();
            physicsComponent->isDynamic = isDynamic;
            physicsComponent->isKinematic = isKinematic;

            if (isDynamic) {
                physicsComponent->actor = physics.CreateRigidDynamic(transform.toPxTransform(), isKinematic);
            }
            // Add static body support later if needed
        }

        void addBoxCollider(Physics& physics, const glm::vec3& size, const glm::vec3& offset = glm::vec3(0.f)) {
            if (!physicsComponent) {
                addPhysicsComponent(physics);
            }

            PxTransform shapeOffset(PxVec3(offset.x, offset.y, offset.z));
            PxMaterial* material = physics.GetDefaultMaterial();

            physicsComponent->shape = physics.CreateBoxShape(size.x, size.y, size.z, shapeOffset, material);

            if (physicsComponent->actor && physicsComponent->shape) {
                physicsComponent->actor->attachShape(*physicsComponent->shape);

                if (physicsComponent->isDynamic) {
                    PxRigidDynamic* dynamicActor = static_cast<PxRigidDynamic*>(physicsComponent->actor);
                    PxRigidBodyExt::updateMassAndInertia(*dynamicActor, physicsComponent->mass);
                }
            }
        }

        void updatePhysics() {
            if (physicsComponent && physicsComponent->actor) {
                transform.updateFromPhysX(physicsComponent->actor);
            }
        }

        void setPhysicsTransform(const glm::vec3& position, const glm::quat& rotation = glm::quat(1, 0, 0, 0)) {
            if (physicsComponent && physicsComponent->actor) {
                PxTransform pxTransform(
                    PxVec3(position.x, position.y, position.z),
                    PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)
                );
                physicsComponent->actor->setGlobalPose(pxTransform);
                transform.translation = position;
                transform.rotation = rotation;
            }
        }

        bool hasPhysics() const { return physicsComponent != nullptr; }

        // Public members
        glm::vec3 color{ 1.f, 1.f, 1.f };
        TransformComponent transform{};
        int imgIndex = 0;
        std::shared_ptr<Model> model{};
        std::unique_ptr<PointLightComponent> pointLight = nullptr;
        std::unique_ptr<PhysicsComponent> physicsComponent = nullptr;

    private:
        GameObject(id_t objId) : id{ objId } {}
        id_t id;
    };
} // namespace grape