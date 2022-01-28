#include "PhysicsComponents.h"

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

	//
	// Create the rigidbody actor!
	// @TODO: start here again, this is where you take the triMesh and connect it to the actor as a shape!!!!
	//
	physx::PxTriangleMeshGeometry triGeom;
	triGeom.triangleMesh = triMesh;
	geom = &triGeom;

	body = PhysicsUtils::createRigidActor(MainLoop::getInstance().physicsPhysics, PhysicsUtils::createTransform(baseObject->getTransform()), rigidActorType);
	shape = physx::PxRigidActorExt::createExclusiveShape(*body, triGeom, *MainLoop::getInstance().defaultPhysicsMaterial);			// @NOTE: When the actor gets released, that's when the exclusiveshape gets released too

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

	//
	// Resize the scale of the triangle mesh
	//
	//body->detachShape(*shape);

	// REDO THE TRI MESH!!!!
}

physx::PxTransform TriangleMeshCollider::getGlobalPose()
{
	return body->getGlobalPose();
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
	body = controller->getActor();

	physx::PxFilterData filterData;
	filterData.word0 = (physx::PxU32)PhysicsUtils::Word0Tags::ENTITY;

	std::vector<physx::PxShape*> shapes;
	shapes.resize(body->getNbShapes());
	int numShapes = body->getShapes(&shapes[0], shapes.size());
	for (size_t i = 0; i < numShapes; i++)
		shapes[i]->setQueryFilterData(filterData);
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

	//// (Last minute) convert -y to y along the face you're sliding down
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

	if (isGrounded && !isSliding)
		cookedVelocity.y = -controller->getStepOffset();

	//
	// Do the deed
	//
	physx::PxControllerCollisionFlags collisionFlags = controller->move(cookedVelocity, 0.01f, MainLoop::getInstance().physicsDeltaTime, NULL, NULL);
	isGrounded = false;
	isSliding = false;
	hasValidStandingOnAngularVelocityY = false;

	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
	{
		isGrounded = true;
		bool isFlatGround = (glm::dot(currentHitNormal, glm::vec3(0, 1, 0)) > controller->getSlopeLimit());

		// Try to retrieve grounded info
		physx::PxRaycastBuffer hitInfo;
		constexpr float padding = 0.05f;
		if (PhysicsUtils::raycast(PhysicsUtils::toPxVec3(controller->getFootPosition()) + physx::PxVec3(0, padding, 0), physx::PxVec3(0, -1, 0), controller->getStepOffset() + padding + padding, hitInfo))
			isFlatGround |= (glm::dot(PhysicsUtils::toGLMVec3(hitInfo.block.normal), glm::vec3(0, 1, 0)) > controller->getSlopeLimit());

		if (isFlatGround)
		{
			velocity.y = 0.0f;		// Remove gravity
			
			if (hitInfo.hasBlock && (hitInfo.block.actor->getType() & physx::PxActorType::eRIGID_DYNAMIC))
			{
				physx::PxRigidDynamic* body = (physx::PxRigidDynamic*)hitInfo.block.actor;
				standingOnAngularVelocityYRadians = body->getAngularVelocity().y;
				hasValidStandingOnAngularVelocityY = true;
			}
		}
		else
			isSliding = true;		// Slide down!
	}

	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
	{
		//std::cout << "\tSide Collision";
	}

	//
	// Check if head/ceiling sliding
	if (collisionFlags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
	{
		if (glm::dot(currentHitNormal, glm::vec3(0, -1, 0)) > controller->getSlopeLimit())
		{
			velocity.y = 0.0f;		// Hit your head on the ceiling
		}
		//else
		//{
		//	// Slide on ceiling!
		//	isCeilingSliding = true;
		//}
	}
	//std::cout << std::endl;

	//
	// Apply transform
	//
	physx::PxExtendedVec3 pos = controller->getPosition();
	glm::mat4 trans = baseObject->getTransform();
	trans[3] = glm::vec4(pos.x, pos.y, pos.z, 1.0f);
	baseObject->INTERNALsubmitPhysicsCalculation(trans);

	//
	// Set the velocity when leaving or landing the ground
	//
	if (prevIsGrounded != isGrounded)
	{
		// Set the velocity!
		velocity = PhysicsUtils::toPxVec3(pos) - prevPositionWhileGrounded;

		if (isGrounded)
		{
			// Landing on the ground
			glm::vec2 flatVelocity(velocity.x, velocity.z);
			float newRunSpeed = glm::length(flatVelocity);

			if (newRunSpeed > 0.01f)
			{
				glm::vec2 idealFacingDirection = glm::normalize(flatVelocity);
				float facingDotIdeal = glm::dot(((PlayerCharacter*)baseObject)->facingDirection, idealFacingDirection);
				if (facingDotIdeal < 0.0f)
				{
					idealFacingDirection *= -1.0f;
					newRunSpeed *= -1.0f;
				}

				((PlayerCharacter*)baseObject)->facingDirection = idealFacingDirection;		// ; TODO: figger out turning speed problems!!! (MAYBE: just have there be an exponential function on turn speed. If moving a certain speed, make turn speed move towards 0 like asymptote)
				((PlayerCharacter*)baseObject)->currentRunSpeed = newRunSpeed;
			}
		}
		else
			// Leaving the ground
			velocity.y = glm::max(velocity.y, 0.0f);
	}
	prevPositionWhileGrounded = PhysicsUtils::toPxVec3(pos);		// Use this to create the velocity
	prevIsGrounded = isGrounded;

	////if (velocity.magnitude() > 100.0f)
	//glm::vec3 flatVelo = PhysicsUtils::toGLMVec3(velocity);
	//flatVelo.y = 0.0f;
	//std::cout << "KOKOMADEDA:\t\t\t" << glm::length(flatVelo) << std::endl;		// ; TODO: start in playercharacter.cpp and see if you can get the grounded mvt and airborne mvt to use the velocity created here as part of their movement!!! Bc it keeps getting reset/capped right at the end of the frame

// @NOTE: Make this not show up in the release game!!!! @DEBUG only... but it's still kinda glitchy so we need it.
//#ifdef _DEVELOP
	// Reset position of player to 0, 0, 0!
	if (InputManager::getInstance().inventoryPressed)
	{
		controller->setFootPosition({ 0, 4, 0 });
		velocity = { 0, 0, 0 };
		((PlayerCharacter*)baseObject)->currentRunSpeed = 0.0f;
	}
//#endif
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
	//hit. @TODO: fimish
	currentHitNormal = glm::vec3(hit.worldNormal.x, hit.worldNormal.y, hit.worldNormal.z);
}
void PlayerPhysics::onControllerHit(const physx::PxControllersHit& hit) { PX_UNUSED(hit); }
void PlayerPhysics::onObstacleHit(const physx::PxControllerObstacleHit& hit) { PX_UNUSED(hit); }

// https://github.com/NVIDIAGameWorks/PhysX-3.4/blob/master/PhysX_3.4/Samples/SampleBridges/SampleBridgesCCT.cpp
physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor)
{
	return physx::PxControllerBehaviorFlag::eCCT_CAN_RIDE_ON_OBJECT;		// NOTE: the physx-provided sliding feature doesn't work so well eh.
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxController&)
{
	return physx::PxControllerBehaviorFlags(0);
}

physx::PxControllerBehaviorFlags PlayerPhysics::getBehaviorFlags(const physx::PxObstacle&)
{
	return physx::PxControllerBehaviorFlags(0);
}
