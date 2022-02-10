#pragma once

#include "../BaseObject.h"
#include "../../utils/PhysicsTypes.h"


class Model;

class TriangleMeshCollider : public PhysicsComponent
{
public:
	TriangleMeshCollider(BaseObject* bo, Model* model, RigidActorTypes rigidActorType, ShapeTypes shapeType = ShapeTypes::COLLISION);
	~TriangleMeshCollider();

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

private:
	physx::PxShape* shape;
	physx::PxGeometry* geom;
	ShapeTypes shapeType;
};

class BoxCollider : public PhysicsComponent
{
public:
	BoxCollider(BaseObject* bo, const glm::vec3& extents, RigidActorTypes rigidActorType, ShapeTypes shapeType = ShapeTypes::COLLISION);
	~BoxCollider();

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

private:
	glm::vec3 extents;
	physx::PxShape* shape;
	physx::PxGeometry* geom;
	ShapeTypes shapeType;
};


class SphereCollider : public PhysicsComponent
{
public:
	SphereCollider(BaseObject* bo, float radius, RigidActorTypes rigidActorType, ShapeTypes shapeType = ShapeTypes::COLLISION);
	~SphereCollider();

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

private:
	physx::PxShape* shape;
	physx::PxGeometry* geom;
	float radius;
	ShapeTypes shapeType;
};


class PlayerPhysics : public PhysicsComponent, public physx::PxUserControllerHitReport, public physx::PxControllerBehaviorCallback
{
public:
	PlayerPhysics(BaseObject* bo);
	~PlayerPhysics();

	void lockVelocity(bool yAlso);

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

	physx::PxVec3 velocity;

	physx::PxCapsuleController* controller;
	physx::PxVec3 tempUp = physx::PxVec3(0.0f, 1.0f, 0.0f);
	
	void setIsGrounded(bool flag) { isGrounded = flag; }
	bool getIsGrounded() { return isGrounded; }
	void setIsSliding(bool flag) { isSliding = flag; }
	bool getIsSliding() { return isSliding; }
	bool getStandingOnAngularVelocityY(float& dipstick) { dipstick = standingOnAngularVelocityYRadians; return hasValidStandingOnAngularVelocityY; }

	// NOTE: this should not be accessed while !isGrounded, bc it isn't updated unless if on ground <45degrees
	glm::vec3 getGroundedNormal() { return groundHitNormal; }

	//
	// PxUserControllerHitReport methods
	//
	virtual void onShapeHit(const physx::PxControllerShapeHit& hit);
	virtual void onControllerHit(const physx::PxControllersHit& hit);
	virtual void onObstacleHit(const physx::PxControllerObstacleHit& hit);

	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxShape& shape, const physx::PxActor& actor);
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxController&);
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags(const physx::PxObstacle&);

private:
	bool isGrounded = false;
	bool isSliding = false;
	bool isSlidingCeiling = false;
	bool isSandwiching = false;		// NOTE: this is where you push yourself into a crevice (defined by collision on bottom and top)
	bool hasValidStandingOnAngularVelocityY = false;
	float standingOnAngularVelocityYRadians;
	glm::vec3 groundHitNormal = glm::vec3(0, 1, 0);
	glm::vec3 ceilingHitNormal = glm::vec3(0, -1, 0);
	physx::PxExtendedVec3 offsetFootMovedReconstructed;		// NOTE: this is only valid/updated when the hitnormal.y >= 0, so essentially if it's a bottom-hit
	physx::PxExtendedVec3 offsetHeadMovedReconstructed;		// NOTE: this is only valid/updated when the hitnormal.y < 0, so essentially if it's a top-hit

	// For retaining velocity when leaving the ground
	bool prevIsGrounded = false;
	physx::PxVec3 prevPositionWhileGrounded;
};
