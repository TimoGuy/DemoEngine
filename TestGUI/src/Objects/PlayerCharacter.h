#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class PlayerImGui : public ImGuiComponent
{
public:
	PlayerImGui(BaseObject* bo) : ImGuiComponent(bo, "Player Controller") {}

	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();
};

class PlayerPhysics : public PhysicsComponent
{
public:
	PlayerPhysics(BaseObject* bo);

	void physicsUpdate(float deltaTime);

	physx::PxCapsuleController* controller;
	physx::PxVec3 tempUp = physx::PxVec3(0.0f, 1.0f, 0.0f);
};

class PlayerRender : public RenderComponent
{
public:
	PlayerRender(BaseObject* bo);

	void render(bool shadowPass, unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture, unsigned int shadowMapTexture);

	unsigned int pbrShaderProgramId, shadowPassSkinnedProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;
};

class PlayerCharacter : public BaseObject
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	ImGuiComponent* imguiComponent;
	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;
};

