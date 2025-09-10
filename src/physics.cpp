#include "physics.hpp"

#include <iostream>

#define PVD_HOST "127.0.0.1"

namespace grape {

	class UserErrorCallback : public PxErrorCallback
	{
	public:
		virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
		{
			std::cout << file << " line " << line << ": " << message << "\n";
			std::cout << "\n";

		}
	}gErrorCallback;

	PxFoundation* _foundation;
	PxDefaultAllocator _allocator;
	PxPvd* _pvd = NULL;
	PxPhysics* _physics = NULL;
	PxDefaultCpuDispatcher* _dispatcher = NULL;
	PxScene* _scene = NULL;
	PxScene* _editorScene = NULL;
	PxMaterial* _defaultMaterial = NULL;
	PxRigidStatic* _groundPlane = NULL;
	PxShape* _groundShape = NULL;

	Physics::Physics() {
		_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _allocator, gErrorCallback);
		if (!_foundation) {
			//PxCreateFoundation failed!
		}

		_pvd = PxCreatePvd(*_foundation);
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
		_pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

		_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, PxTolerancesScale(), true, _pvd);
		if (!_physics) {
			//PxCreatePhysics failed!
		}

		PxSceneDesc sceneDesc(_physics->getTolerancesScale());
		sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
		_dispatcher = PxDefaultCpuDispatcherCreate(2);
		sceneDesc.cpuDispatcher = _dispatcher;
		sceneDesc.filterShader = PxDefaultSimulationFilterShader;
		_scene = _physics->createScene(sceneDesc);
		_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
		_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);

		PxPvdSceneClient* pvdClient = _scene->getScenePvdClient();
		if (pvdClient) {
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		_defaultMaterial = _physics->createMaterial(0.5f, 0.5f, 0.6f);

		_groundPlane = PxCreatePlane(*_physics, PxPlane(0, 1, 0, 1), *_defaultMaterial);
		_scene->addActor(*_groundPlane);
		_groundPlane->getShapes(&_groundShape, 1);
	}

	Physics::~Physics() {
	}

	void Physics::StepPhysics(float deltaTime)
	{
		_scene->simulate(deltaTime);
		_scene->fetchResults(true);
	}

	PxConvexMesh* Physics::CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices) {
		PxConvexMeshDesc convexDesc;
		convexDesc.points.count = numVertices;
		convexDesc.points.stride = sizeof(PxVec3);
		convexDesc.points.data = vertices;
		convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
		//  s
		PxTolerancesScale scale;
		PxCookingParams params(scale);

		PxDefaultMemoryOutputStream buf;
		PxConvexMeshCookingResult::Enum result;
		if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
			return NULL;
		}
		PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
		return _physics->createConvexMesh(input);
	}

	PxRigidDynamic* Physics::CreateRigidDynamic(PxTransform transform, bool kinematic)
	{
		/*PxMat44 mat;;
		for (int x = 0; x < 4; x++)
			for (int y = 0; y < 4; y++)
				mat[x][y] = matrix[x][y];

		PxTransform transform(mat);*/
		PxRigidDynamic* body = _physics->createRigidDynamic(transform);
		body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
		_scene->addActor(*body);
		return body;
	}

	PxShape* Physics::CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags, PxMaterial* material, glm::vec3 scale)
	{
		//if (material == NULL) {
		//	material = _defaultMaterial;
		//}
		////PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eTIGHT_BOUNDS | ~PxMeshGeometryFlag::eDOUBLE_SIDED);
		//// maybe tight bounds did something?? check it out...
		//PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eDOUBLE_SIDED);
		//PxTriangleMeshGeometry geometry(triangleMesh, PxMeshScale(PxVec3(scale.x, scale.y, scale.z)), flags);

		//PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
		//return _physics->createShape(geometry, *material, shapeFlags);
		return nullptr;
	}

	PxShape* Physics::CreateBoxShape(float width, float height, float depth, PxTransform shapeOffset, PxMaterial* material) {
		if (material == NULL) {
			material = _defaultMaterial;
		}
		PxShape* shape = _physics->createShape(PxBoxGeometry(width, height, depth), *material, true);
		//PxMat44 localShapeMatrix = GlmMat4ToPxMat44(shapeOffset.mat4());
		//PxTransform localShapeTransform(localShapeMatrix);
		shape->setLocalPose(shapeOffset);
		return shape;
	}

	PxRigidDynamic* Physics::CreateRigidDynamic(glm::mat4 matrix, PxShape* shape) {
		PxFilterData filterData;
		filterData.word0 = (PxU32)0;
		filterData.word1 = (PxU32)0;
		filterData.word2 = (PxU32)4;
		shape->setQueryFilterData(filterData);       // ray casts
		shape->setSimulationFilterData(filterData);  // collisions
		PxMat44 mat = GlmMat4ToPxMat44(matrix);
		PxTransform transform(mat);
		PxRigidDynamic* body = _physics->createRigidDynamic(transform);
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		_scene->addActor(*body);
		return body;
	}

	inline PxMat44 Physics::GlmMat4ToPxMat44(glm::mat4 glmMatrix) {
		PxMat44 matrix;
		for (int x = 0; x < 4; x++)
			for (int y = 0; y < 4; y++)
				matrix[x][y] = glmMatrix[x][y];
		return matrix;
	}

	PxMaterial* Physics::GetDefaultMaterial() {
		return _defaultMaterial;
	}
}
