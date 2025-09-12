#pragma once
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <glm\glm.hpp>
#include <vector>

using namespace physx;

namespace grape {
    // Forward declaration for FrameInfo
    struct FrameInfo;

    struct PhysicsDebugLine {
        glm::vec3 start;
        glm::vec3 end;
        glm::vec3 color;
    };

    class Physics {
    public:
        Physics();
        ~Physics();
        void StepPhysics(float deltaTime);
        inline PxMat44 GlmMat4ToPxMat44(glm::mat4 glmMatrix);
        PxRigidDynamic* CreateRigidDynamic(PxTransform transform, bool kinematic);
        PxShape* CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags, PxMaterial* material = NULL, glm::vec3 scale = glm::vec3(1));
        PxConvexMesh* CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices);
        PxShape* CreateBoxShape(float width, float height, float depth, PxTransform transform, PxMaterial* material);
        PxMaterial* GetDefaultMaterial();
        PxRigidDynamic* CreateRigidDynamic(glm::mat4 matrix, PxShape* shape);

        // Debug functionality
        void setDebugVisualization(bool enabled);
        void updateDebugData();
        void renderDebugLines(FrameInfo& frameInfo);
        const std::vector<PhysicsDebugLine>& getDebugLines() const { return debugLines; }

        // Getters for debug info
        PxU32 getActiveBodyCount() const;
        PxU32 getContactCount() const;

    private:
        std::vector<PhysicsDebugLine> debugLines;
        bool debugVisualizationEnabled = false;
    };
}