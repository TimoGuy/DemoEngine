#include "PhysicsComponents.h"

#include <glm/gtx/norm.hpp>
#include "../../mainloop/MainLoop.h"
#include "../../utils/PhysicsUtils.h"
#include "../../utils/GameState.h"
#include "../../render_engine/model/Model.h"
#include "../PlayerCharacter.h"

//#ifdef _DEVELOP
#include "../../utils/InputManager.h"
//#endif


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// TriangleMeshCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
TriangleMeshCollider::TriangleMeshCollider(BaseObject* bo, std::vector<ModelWithTransform> models, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), models(models), rigidActorType(rigidActorType)
{
	routineCreateTriangleMeshGeometry(baseObject->getTransform());
}

TriangleMeshCollider::~TriangleMeshCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void TriangleMeshCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void TriangleMeshCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 newScale = PhysicsUtils::getScale(newTransform);
	if (newScale != cachedScale)
	{
		// REDO THE TRI MESH!!!!
		routineCreateTriangleMeshGeometry(newTransform);
	}
	else
	{
		// NOTE: You shouldn't need to recreate the mesh if scale isn't affected
		physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
		body->setGlobalPose(trans);
	}
}

physx::PxTransform TriangleMeshCollider::getGlobalPose()
{
	return body->getGlobalPose();
}

void TriangleMeshCollider::routineCreateTriangleMeshGeometry(const glm::mat4& newTransform)
{
	if (body != nullptr)
	{
		MainLoop::getInstance().physicsScene->removeActor(*body);
		body->release();
		body = nullptr;
	}

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);

	physx::PxU32 nbVerts = 0;
	physx::PxU32 nbIndices = 0;
	uint32_t baseIndex = 0;

	std::vector<physx::PxVec3> verts;
	std::vector<physx::PxU32> indices32;

	//
	// Extract renderMeshes from model and conform to physx trianglemeshdesc
	//
	glm::vec3 xformScale = PhysicsUtils::getScale(newTransform);
	for (auto& model : models)
	{
		const std::vector<Mesh>& modelMeshes = model.model->getPhysicsMeshes();
		for (size_t i = 0; i < modelMeshes.size(); i++)
		{
			const std::vector<Vertex>& vertices = modelMeshes[i].getVertices();
			const std::vector<uint32_t>& indices = modelMeshes[i].getIndices();

			nbVerts += (physx::PxU32)vertices.size();
			nbIndices += (physx::PxU32)indices.size();

			//
			// Add in vertices
			//
			for (size_t j = 0; j < vertices.size(); j++)
			{
				const glm::vec3 vec = glm::vec3(model.localTransform * glm::vec4(vertices[j].position, 1.0f)) * xformScale;
				verts.push_back(physx::PxVec3(vec.x, vec.y, vec.z));
			}

			//
			// Add in indices
			//
			for (size_t j = 0; j < indices.size(); j++)
			{
				indices32.push_back(indices[j] + baseIndex);
			}

			// Bump counter
			baseIndex += (uint32_t)vertices.size();
		}
	}

	physx::PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = nbVerts;
	meshDesc.points.stride = sizeof(physx::PxVec3);
	meshDesc.points.data = &verts[0];
	
	meshDesc.triangles.count = nbIndices / 3;
	meshDesc.triangles.stride = 3 * sizeof(physx::PxU32);
	meshDesc.triangles.data = &indices32[0];

	physx::PxDefaultMemoryOutputStream writeBuffer;
	physx::PxTriangleMeshCookingResult::Enum result;
	bool status = MainLoop::getInstance().physicsCooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if (!status)
		return;

	physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	physx::PxTriangleMesh* triMesh = MainLoop::getInstance().physicsPhysics->createTriangleMesh(readBuffer);

	physx::PxTriangleMeshGeometry triGeom;
	triGeom.triangleMesh = triMesh;
	geom = &triGeom;

	//
	// Create the rigidbody actor!
	// @TODO: start here again, this is where you take the triMesh and connect it to the actor as a shape!!!!
	//
	shape = physx::PxRigidActorExt::createExclusiveShape(*body, *geom, *MainLoop::getInstance().defaultPhysicsMaterial);			// @NOTE: When the actor gets released, that's when the exclusiveshape gets released too

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));
	MainLoop::getInstance().physicsScene->addActor(*body);
	triMesh->release();
	cachedScale = xformScale;
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// BoxCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
BoxCollider::BoxCollider(BaseObject* bo, const glm::vec3& extents, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), extents(extents), shapeType(shapeType)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->getTransform());

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	glm::vec3 realExtents = extents * scale;

	geom = new physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));

	MainLoop::getInstance().physicsScene->addActor(*body);
	shape->release();
}

BoxCollider::~BoxCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void BoxCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void BoxCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 scale = PhysicsUtils::getScale(newTransform);
	glm::vec3 realExtents = extents * scale;

	//
	// TODO: change to: Get, then move shape
	// NOTE: apparently it doesn't work when trying to set the geometry
	//
	body->detachShape(*shape);

	geom = new physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);
	shape->release();

	physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
	body->setGlobalPose(trans);
}

physx::PxTransform BoxCollider::getGlobalPose() { return body->getGlobalPose(); }


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// SphereCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
SphereCollider::SphereCollider(BaseObject* bo, float radius, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), radius(radius), shapeType(shapeType)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->getTransform());

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	float maxScale = std::max(scale.x, std::max(scale.y, scale.z));

	geom = new physx::PxSphereGeometry(maxScale * radius);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));

	MainLoop::getInstance().physicsScene->addActor(*body);
	shape->release();
}

SphereCollider::~SphereCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void SphereCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void SphereCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 scale = PhysicsUtils::getScale(newTransform);
	float maxScale = std::max(scale.x, std::max(scale.y, scale.z));

	//
	// TODO: change to: Get, then move shape
	//
	body->detachShape(*shape);

	geom = new physx::PxSphereGeometry(maxScale * radius);
	shape = MainLoop::getInstance().physicsPhysics->createShape(*geom, *MainLoop::getInstance().defaultPhysicsMaterial);

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::UNTAGGED;
	shape->setQueryFilterData(filterData);

	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}
	body->attachShape(*shape);
	shape->release();

	physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
	body->setGlobalPose(trans);
}

physx::PxTransform SphereCollider::getGlobalPose() { return body->getGlobalPose(); }
