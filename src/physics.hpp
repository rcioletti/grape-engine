#pragma once

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <glm\glm.hpp>

using namespace physx;

namespace grape {
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
	};
}