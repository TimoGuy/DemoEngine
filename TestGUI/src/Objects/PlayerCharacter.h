#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class PlayerCharacter : public ImGuiObject, public PhysicsObject, public RenderObject
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	void physicsUpdate(float deltaTime);
	void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture);
	void propertyPanelImGui();
	void renderImGui();

private:
	unsigned int pbrShaderProgramId, shadowPassSkinnedProgramId;

	physx::PxRigidDynamic* body;
	physx::PxTransform transform;
	physx::PxBoxGeometry boxCollider;
	physx::PxSphereGeometry sphereCollider;
	physx::PxCapsuleGeometry capsuleCollider;
	physx::PxCapsuleController* controller;
	bool reapplyTransform;

	physx::PxVec3 tempUp = physx::PxVec3(0.0f, 1.0f, 0.0f);

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;
};

