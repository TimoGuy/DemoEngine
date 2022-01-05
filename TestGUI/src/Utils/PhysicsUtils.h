#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <PxPhysicsAPI.h>

#ifdef _DEVELOP
#include "../imgui/imgui.h"
#endif

#include "PhysicsTypes.h"

struct RenderAABB;

namespace PhysicsUtils
{
#pragma region Factory functions

	physx::PxVec3 toPxVec3(const physx::PxExtendedVec3& in);
	physx::PxVec3 toPxVec3(const glm::vec3& in);
	glm::vec3 toGLMVec3(const physx::PxVec3& in);

	physx::PxQuat createQuatFromEulerDegrees(glm::vec3 eulerAnglesDegrees);

	glm::mat4 createGLMTransform(glm::vec3 position, glm::vec3 eulerAnglesDegrees = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f));

	physx::PxTransform createTransform(glm::vec3 position, glm::vec3 eulerAnglesDegrees = glm::vec3(0.0f));

	physx::PxTransform createTransform(glm::mat4 transform);

	glm::mat4 physxTransformToGlmMatrix(physx::PxTransform transform);

	physx::PxRigidActor* createRigidActor(physx::PxPhysics* physics, physx::PxTransform transform, RigidActorTypes rigidActorType);

	//physx::PxBoxGeometry createBoxCollider;				// TODO: Idk if these functions would be worth it to build... let's just keep going and see if they are
	//physx::PxSphereGeometry sphereCollider;
	//physx::PxCapsuleGeometry capsuleCollider;

	physx::PxCapsuleController* createCapsuleController(
		physx::PxControllerManager* controllerManager,
		physx::PxMaterial* physicsMaterial,
		physx::PxExtendedVec3 position,
		float radius,
		float height,
		physx::PxUserControllerHitReport* hitReport = nullptr,
		physx::PxControllerBehaviorCallback* behaviorReport = nullptr,
		float slopeLimit = 0.70710678118f,						// cosine of 45 degrees
		physx::PxVec3 upDirection = physx::PxVec3(0, 1, 0));

	float moveTowards(float current, float target, float maxDistanceDelta);
	float smoothStep(float edge0, float edge1, float t);
	glm::i64 moveTowards(glm::i64 current, glm::i64 target, glm::i64 maxDistanceDelta);
	float moveTowardsAngle(float currentAngle, float targetAngle, float maxTurnDelta);
	glm::vec2 clampVector(glm::vec2 vector, float min, float max);
	bool raycast(physx::PxVec3 origin, physx::PxVec3 unitDirection, physx::PxReal distance, physx::PxRaycastBuffer& hitInfo);

#pragma endregion

#pragma region simple glm decomposition functions

	glm::vec3 getPosition(glm::mat4 transform);

	glm::quat getRotation(glm::mat4 transform);

	glm::vec3 getScale(glm::mat4 transform);

#pragma endregion

#ifdef _DEVELOP
#pragma region imgui property panel functions

	void imguiTransformMatrixProps(float* matrixPtr);

#pragma endregion

#pragma region imgui draw functions

	void imguiRenderBoxCollider(glm::mat4 modelMatrix, physx::PxBoxGeometry& boxGeometry, ImU32 color = ImColor::HSV(0.39f, 0.88f, 0.92f));

	void imguiRenderSphereCollider(glm::mat4 modelMatrix, physx::PxSphereGeometry& sphereGeometry);

	void imguiRenderCapsuleCollider(glm::mat4 modelMatrix, physx::PxCapsuleGeometry& capsuleGeometry);

	void imguiRenderCharacterController(glm::mat4 modelMatrix, physx::PxCapsuleController& controller);

	void imguiRenderCircle(glm::mat4 modelMatrix, float radius, glm::vec3 eulerAngles, glm::vec3 offset, unsigned int numVertices, ImU32 color = ImColor::HSV(0.39f, 0.88f, 0.92f));

	void imguiRenderSausage(glm::mat4 modelMatrix, float radius, float halfHeight, glm::vec3 eulerAngles, unsigned int numVertices);

#pragma endregion
#endif

	struct RaySegmentHit
	{
		bool hit;
		glm::vec3 hitPositionWorldSpace;
		float distance;
	};

	RenderAABB fitAABB(RenderAABB bounds, glm::mat4 modelMatrix);
	RaySegmentHit raySegmentCollideWithAABB(glm::vec3 start, glm::vec3 end, RenderAABB bounds);

	float lerp(float a, float b, float t);
	glm::vec3 lerp(glm::vec3 a, glm::vec3 b, glm::vec3 t);
	float lerpAngleDegrees(float a, float b, float t);
}