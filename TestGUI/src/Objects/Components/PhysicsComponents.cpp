#include "PhysicsComponents.h"

#include "../../MainLoop/MainLoop.h"
#include "../../Utils/PhysicsUtils.h"


// -----------------------------------------------------------------------------------------------------------------------------------------------------------
// BoxCollider Class
// -----------------------------------------------------------------------------------------------------------------------------------------------------------
BoxCollider::BoxCollider(BaseObject* bo, Bounds* bounds, RigidActorTypes rigidActorType, ShapeTypes shapeType) : PhysicsComponent(bo), bounds(bounds), shapeType(shapeType)
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
			this		// PxUserControllerHitReport
		);
}

PlayerPhysics::~PlayerPhysics()
{
	controller->release();
}

void PlayerPhysics::physicsUpdate()
{
	//
	// Add gravity (or sliding gravity if sliding)
	//
	velocity.y -= 9.8f * MainLoop::getInstance().physicsDeltaTime;

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

	//// @Checkin
	if (hit.worldNormal.dot(physx::PxVec3(0, 1, 0)) <= 0.707106781f)		// NOTE: 0.7... is cos(45deg)
	{
		physx::PxVec3 dtiith = hit.dir;
		float jjjjj = hit.length;
	}
}
void PlayerPhysics::onControllerHit(const physx::PxControllersHit& hit) { PX_UNUSED(hit); }
void PlayerPhysics::onObstacleHit(const physx::PxControllerObstacleHit& hit) { PX_UNUSED(hit); }
