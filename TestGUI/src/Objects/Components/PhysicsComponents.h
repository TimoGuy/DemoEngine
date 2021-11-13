#pragma once

#include "../BaseObject.h"
#include "../../Utils/PhysicsTypes.h"


class BoxCollider : public PhysicsComponent
{
public:
	BoxCollider(BaseObject* bo, Bounds* bounds, RigidActorTypes rigidActorType, ShapeTypes shapeType = ShapeTypes::COLLISION);
	~BoxCollider();

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

	physx::PxBoxGeometry getBoxGeometry();

private:
	Bounds* bounds;
	physx::PxShape* shape;
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

	physx::PxSphereGeometry getSphereGeometry();

private:
	physx::PxShape* shape;
	float radius;
	ShapeTypes shapeType;
};
