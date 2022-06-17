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
	void routineCreateTriangleMeshGeometry(const glm::mat4& newTransform);

	Model* model;
	physx::PxShape* shape;
	physx::PxGeometry* geom;
	RigidActorTypes rigidActorType = RigidActorTypes::STATIC;
	ShapeTypes shapeType = ShapeTypes::COLLISION;
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
	ShapeTypes shapeType = ShapeTypes::COLLISION;
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
	ShapeTypes shapeType = ShapeTypes::COLLISION;
};
