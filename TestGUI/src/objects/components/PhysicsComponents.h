#pragma once

#include "../BaseObject.h"
#include "../../utils/PhysicsTypes.h"


class Model;

struct ModelWithTransform
{
	Model* model = nullptr;
	glm::mat4 localTransform = glm::mat4(1.0f);
};

class TriangleMeshCollider : public PhysicsComponent
{
public:
	TriangleMeshCollider(BaseObject* bo, std::vector<ModelWithTransform> models, RigidActorTypes rigidActorType, ShapeTypes shapeType = ShapeTypes::COLLISION);
	~TriangleMeshCollider();

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

private:
	void routineCreateTriangleMeshGeometry(const glm::mat4& newTransform);

	std::vector<ModelWithTransform> models;
	physx::PxShape* shape;
	physx::PxGeometry* geom;
	RigidActorTypes rigidActorType = RigidActorTypes::STATIC;
	ShapeTypes shapeType = ShapeTypes::COLLISION;

#ifdef _DEVELOP
	glm::vec3 cachedScale;
#endif
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
