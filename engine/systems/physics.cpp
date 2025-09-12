#include "physics.hpp"
#include "renderer/frame_info.hpp"  // Add this include

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

		// Initialize debug visualization (disabled by default)
		setDebugVisualization(false);

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

		// Update debug data if visualization is enabled
		if (debugVisualizationEnabled) {
			updateDebugData();
		}
	}

	void Physics::setDebugVisualization(bool enabled) {
		debugVisualizationEnabled = enabled;

		if (!_scene) return;

		if (enabled) {
			// Enable debug visualization
			_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_AABBS, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_FORCE, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
			_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);

			std::cout << "Physics Debug Visualization: ENABLED" << std::endl;
		}
		else {
			// Disable debug visualization
			_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
			debugLines.clear();
			std::cout << "Physics Debug Visualization: DISABLED" << std::endl;
		}
	}

	void Physics::updateDebugData() {
		debugLines.clear();

		if (!_scene || !debugVisualizationEnabled) return;

		// Get debug render data from PhysX
		const PxRenderBuffer& renderBuffer = _scene->getRenderBuffer();

		// Convert PhysX debug lines to our format
		const PxDebugLine* lines = renderBuffer.getLines();
		PxU32 numLines = renderBuffer.getNbLines();

		for (PxU32 i = 0; i < numLines; i++) {
			const PxDebugLine& line = lines[i];

			PhysicsDebugLine debugLine;
			debugLine.start = glm::vec3(line.pos0.x, line.pos0.y, line.pos0.z);
			debugLine.end = glm::vec3(line.pos1.x, line.pos1.y, line.pos1.z);

			// Convert PhysX color to glm::vec3 (ARGB to RGB)
			PxU32 color = line.color0;
			debugLine.color = glm::vec3(
				((color >> 16) & 0xFF) / 255.0f,  // Red
				((color >> 8) & 0xFF) / 255.0f,   // Green
				(color & 0xFF) / 255.0f           // Blue
			);

			debugLines.push_back(debugLine);
		}

		// Also get triangles for solid shapes and convert to wireframe
		const PxDebugTriangle* triangles = renderBuffer.getTriangles();
		PxU32 numTriangles = renderBuffer.getNbTriangles();

		for (PxU32 i = 0; i < numTriangles; i++) {
			const PxDebugTriangle& tri = triangles[i];

			PxU32 color = tri.color0;
			glm::vec3 wireColor = glm::vec3(
				((color >> 16) & 0xFF) / 255.0f,
				((color >> 8) & 0xFF) / 255.0f,
				(color & 0xFF) / 255.0f
			);

			// Create 3 lines for triangle wireframe
			PhysicsDebugLine line1, line2, line3;

			line1.start = glm::vec3(tri.pos0.x, tri.pos0.y, tri.pos0.z);
			line1.end = glm::vec3(tri.pos1.x, tri.pos1.y, tri.pos1.z);
			line1.color = wireColor;

			line2.start = glm::vec3(tri.pos1.x, tri.pos1.y, tri.pos1.z);
			line2.end = glm::vec3(tri.pos2.x, tri.pos2.y, tri.pos2.z);
			line2.color = wireColor;

			line3.start = glm::vec3(tri.pos2.x, tri.pos2.y, tri.pos2.z);
			line3.end = glm::vec3(tri.pos0.x, tri.pos0.y, tri.pos0.z);
			line3.color = wireColor;

			debugLines.push_back(line1);
			debugLines.push_back(line2);
			debugLines.push_back(line3);
		}
	}

	void Physics::renderDebugLines(FrameInfo& frameInfo) {
		// For now, this is a placeholder
		// You'll need to implement line rendering in your Vulkan pipeline
		// This could be a future enhancement

		// Temporary: Just print debug info
		if (debugVisualizationEnabled && !debugLines.empty()) {
			static int frameCounter = 0;
			if (++frameCounter % 120 == 0) {  // Every 2 seconds at 60fps
				std::cout << "Physics Debug: " << debugLines.size() << " debug lines" << std::endl;
			}
		}
	}

	PxU32 Physics::getActiveBodyCount() const {
		if (!_scene) return 0;
		return _scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC);
	}

	PxU32 Physics::getContactCount() const {
		// This is more complex to implement, return 0 for now
		return 0;
	}

	// ... rest of your existing methods remain unchanged ...

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
		PxRigidDynamic* body = _physics->createRigidDynamic(transform);
		body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
		_scene->addActor(*body);
		return body;
	}

	PxShape* Physics::CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags, PxMaterial* material, glm::vec3 scale)
	{
		return nullptr;
	}

	PxShape* Physics::CreateBoxShape(float width, float height, float depth, PxTransform shapeOffset, PxMaterial* material) {
		if (material == NULL) {
			material = _defaultMaterial;
		}
		PxShape* shape = _physics->createShape(PxBoxGeometry(width, height, depth), *material, true);
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