#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <PxPhysicsAPI.h>


namespace PhysicsUtils
{
#pragma region Factory functions

	physx::PxTransform createTransform(glm::vec3 position, glm::vec3 eulerAngles = glm::vec3(0.0f));

	physx::PxRigidDynamic* createRigidbodyDynamic(physx::PxPhysics* physics, physx::PxTransform transform);

	physx::PxRigidStatic* createRigidbodyStatic(physx::PxPhysics* physics, physx::PxTransform transform);

	//physx::PxBoxGeometry createBoxCollider;				// TODO: Idk if these functions would be worth it to build... let's just keep going and see if they are
	//physx::PxSphereGeometry sphereCollider;
	//physx::PxCapsuleGeometry capsuleCollider;

	physx::PxCapsuleController* createCapsuleController(
		physx::PxControllerManager* controllerManager,
		physx::PxMaterial* physicsMaterial,
		physx::PxExtendedVec3 position,
		float radius,
		float height,
		float slopeLimit = 0.70710678118f,						// cosine of 45 degrees
		physx::PxVec3 upDirection = physx::PxVec3(0, 1, 0));

#pragma endregion

#pragma region simple glm decomposition functions

	glm::vec3 getPosition(glm::mat4 transform);

	glm::quat getRotation(glm::mat4 transform);

#pragma endregion

#pragma region imgui property panel functions

	void imguiTransformMatrixProps(float* matrixPtr);

#pragma endregion

#pragma region imgui draw functions

	void imguiRenderBoxCollider(glm::mat4 modelMatrix, physx::PxBoxGeometry& boxGeometry);

	void imguiRenderSphereCollider(glm::mat4 modelMatrix, physx::PxSphereGeometry& sphereGeometry);

	void imguiRenderCapsuleCollider(glm::mat4 modelMatrix, physx::PxCapsuleGeometry& capsuleGeometry);

	void imguiRenderCharacterController(glm::mat4 modelMatrix, physx::PxCapsuleController& controller);

	void imguiRenderCircle(glm::mat4 modelMatrix, float radius, glm::vec3 eulerAngles, glm::vec3 offset, unsigned int numVertices);

	void imguiRenderSausage(glm::mat4 modelMatrix, float radius, float halfHeight, glm::vec3 eulerAngles, unsigned int numVertices);

#pragma endregion
}