#include "PhysicsComponents.h"

#include "../../MainLoop/MainLoop.h"
#include "../../Utils/PhysicsUtils.h"
#include "../../Utils/GameState.h"
#include "../../RenderEngine/RenderEngine.model/Model.h"


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// TriangleMeshCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
TriangleMeshCollider::TriangleMeshCollider(BaseObject* bo, Model* model, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo)
{
	physx::PxU32 nbVerts = 0;
	physx::PxU32 nbIndices = 0;
	uint32_t baseIndex = 0;

	std::vector<physx::PxVec3> verts;
	std::vector<physx::PxU32> indices32;

	//
	// Extract meshes from model and conform to physx trianglemeshdesc
	//
	const std::vector<Mesh>& modelMeshes = model->getMeshes();
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
			const glm::vec3 vec = vertices[j].position;
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

	// Get the bounds of the mesh and pass on to the physical bounds
	physx::PxBounds3 boundsFromMesh = triMesh->getLocalBounds();
	baseObject->getImguiComponent()->bounds->center = PhysicsUtils::toGLMVec3(boundsFromMesh.getCenter());
	baseObject->getImguiComponent()->bounds->extents = PhysicsUtils::toGLMVec3(boundsFromMesh.getExtents());

	//
	// Create the rigidbody actor!
	// @TODO: start here again, this is where you take the triMesh and connect it to the actor as a shape!!!!
	//
	physx::PxTriangleMeshGeometry triGeom;
	triGeom.triangleMesh = triMesh;

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	shape = physx::PxRigidActorExt::createExclusiveShape(*body, triGeom, *MainLoop::getInstance().defaultPhysicsMaterial);			// @NOTE: @MemLeak: @Kakunin: This may be a mem leak... but it appears that exclusive shapes should not get released bc when the actor is released that's when the shape gets released.
	if (shapeType == ShapeTypes::TRIGGER)
	{
		shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shape->setFlag(physx::PxShapeFlag::eTRIGGER_SHAPE, true);
	}

	body->setGlobalPose(PhysicsUtils::createTransform(baseObject->getTransform()));

	MainLoop::getInstance().physicsScene->addActor(*body);
	triMesh->release();
}

TriangleMeshCollider::~TriangleMeshCollider()
{
	MainLoop::getInstance().physicsScene->removeActor(*body);
	body->release();
}

void TriangleMeshCollider::physicsUpdate() { baseObject->physicsUpdate(); }

void TriangleMeshCollider::propagateNewTransform(const glm::mat4& newTransform)
{
	// NOTE: I guess that it doesn't matter that you recreate the shape geometry huh. (Why do I have to do it for boxCollider and Spherecollider... maybe it's the scale???)
	physx::PxTransform trans = PhysicsUtils::createTransform(newTransform);
	body->setGlobalPose(trans);
}

physx::PxTransform TriangleMeshCollider::getGlobalPose()
{
	return body->getGlobalPose();
}

// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// BoxCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
BoxCollider::BoxCollider(BaseObject* bo, RenderAABB* bounds, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), bounds(bounds), shapeType(shapeType)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->getTransform());

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	glm::vec3 realExtents = bounds->extents * scale;

	shape = MainLoop::getInstance().physicsPhysics->createShape(physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z), *MainLoop::getInstance().defaultPhysicsMaterial);
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
	glm::vec3 realExtents = bounds->extents * scale;

	//
	// TODO: change to: Get, then move shape
	// NOTE: apparently it doesn't work when trying to set the geometry
	//
	body->detachShape(*shape);
	shape = MainLoop::getInstance().physicsPhysics->createShape(physx::PxBoxGeometry(realExtents.x, realExtents.y, realExtents.z), *MainLoop::getInstance().defaultPhysicsMaterial);
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

physx::PxBoxGeometry BoxCollider::getBoxGeometry()
{
	physx::PxBoxGeometry geom;
	shape->getBoxGeometry(geom);
	return geom;
}


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// SphereCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
SphereCollider::SphereCollider(BaseObject* bo, float radius, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), radius(radius), shapeType(shapeType)
{
	glm::vec3 scale = PhysicsUtils::getScale(baseObject->getTransform());

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	float maxScale = std::max(scale.x, std::max(scale.y, scale.z));

	shape = MainLoop::getInstance().physicsPhysics->createShape(physx::PxSphereGeometry(maxScale * radius), *MainLoop::getInstance().defaultPhysicsMaterial);
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
	shape = MainLoop::getInstance().physicsPhysics->createShape(physx::PxSphereGeometry(maxScale * radius), *MainLoop::getInstance().defaultPhysicsMaterial);
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

physx::PxSphereGeometry SphereCollider::getSphereGeometry()
{
	physx::PxSphereGeometry geom;
	shape->getSphereGeometry(geom);
	return geom;
}


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// PlayerPhysics Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
PlayerPhysics::PlayerPhysics(BaseObject* bo) : PhysicsComponent(bo)
{
	controller =
		PhysicsUtils::createCapsuleController(
			MainLoop::getInstance().physicsControllerManager,
			MainLoop::getInstance().defaultPhysicsMaterial,
			physx::PxExtendedVec3(0.0f, 100.0f, 0.0f),
			1.0f,
			4.5f,
			this,		// PxUserControllerHitReport
			this		// PxBehaviorCallbackWhatevs
		);

	GameState::getInstance().playerActorPointer = controller->getActor();
}

PlayerPhysics::~PlayerPhysics()
{
	controller->release();
}

void PlayerPhysics::lockVelocity(bool yAlso)
{
	if (yAlso)
		velocity = physx::PxVec3(0);
	else
	{
		velocity.x = 0;
		velocity.z = 0;
	}
}

void PlayerPhysics::physicsUpdate()
{
	//
	// Add gravity (or sliding gravity if sliding)
	//
	velocity.y -= 4.9f * MainLoop::getInstance().physicsDeltaTime;

	physx::PxVec3 cookedVelocity = velocity;

	// (Last minute) convert -y to y along the face you're sliding down
	if (isSliding)
	{
		const glm::vec3 upXnormal = glm::cross(glm::vec3(0, 1, 0), currentHitNormal);
		const glm::vec3 uxnXnormal = glm::normalize(glm::cross(upXnormal, currentHitNormal));
		const glm::vec3 slidingVector = uxnXnormal * -velocity.y;

		const float flatSlidingUmph = 0.9f;			// NOTE: this is so that it's guaranteed that the character will also hit the ground the next frame, thus keeping the sliding state
		cookedVelocity.y = 0.0f;
		cookedVelocity += physx::PxVec3(
			slidingVector.x * flatSlidingUmph,
			slidingVector.y,
			slidingVector.z * flatSlidingUmph
		);
	}

	//
	// Do the deed
	//
	physx::PxControllerCollisionFlags collisionFlags = controller->move(cookedVelocity, 0.01f, MainLoop::getInstance().physicsDeltaTime, NULL, NULL);
	isGrounded = false;
	isSliding = false;

	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		isGrounded = true;
		if (glm::dot(currentHitNormal, glm::vec3(0, 1, 0)) > 0.707106781f)		// NOTE: 0.7... is cos(45deg)
		{
			velocity.y = 0.0f;		// Remove gravity
		}
		else
		{
			// Slide down!
			isSliding = true;
		}
	}
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
	{
		//std::cout << "\tSide Collision";
	}
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		//std::cout << "\tAbove Collision";
		velocity.y = 0.0f;		// Hit your head on the ceiling
	}
	//std::cout << std::endl;

	//
	// Apply transform
	//
	physx::PxExtendedVec3 pos = controller->getPosition();
	glm::mat4 trans = baseObject->getTransform();
	trans[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
	baseObject->INTERNALsubmitPhysicsCalculation(trans);
}

void PlayerPhysics::propagateNewTransform(const glm::mat4& newTransform)
{
	glm::vec3 pos = PhysicsUtils::getPosition(newTransform);
	controller->setPosition(physx::PxExtendedVec3(pos.x, pos.y, pos.z));
}

physx::PxTransform PlayerPhysics::getGlobalPose()
{
	physx::PxExtendedVec3 pos = controller->getPosition();
	return PhysicsUtils::createTransform(glm::vec3(pos.x, pos.y, pos.z));
}

void PlayerPhysics::onShapeHit(const physx::PxControllerShapeHit& hit)
{
	currentHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
}
void PlayerPhysics::onControllerHit(const physx::PxControllersHit& hit) { PX_UNUSED(hit); }
void PlayerPhysics::onObstacleHit(const physx::PxControllerObstacleHit& hit) { PX_UNUSED(hit); }

// https://github.com/NVIDIAGameWorks/PhysX-3.4/blob/master/PhysX_3.4/Samples/SampleBridges/SampleBridgesCCT.cpp
physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor)
{
	// PT: ride & slide on platforms
	return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;// | physx::PxControllerBehaviorFlag::eCCT_SLIDE;

	//return PxControllerBehaviorFlags(0);
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxController&)
{
	return physx::PxControllerBehaviorFlags(0);
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxObstacle&)
{
	return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT | physx::PxControllerBehaviorFlag::eCCT_SLIDE;
}
