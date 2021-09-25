#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"


class PlayerImGui : public ImGuiComponent
{
public:
	PlayerImGui(BaseObject* bo, Bounds* bounds) : ImGuiComponent(bo, bounds, "Player Controller") {}

	void propertyPanelImGui();
	void renderImGui();
	void cloneMe();
};

class PlayerPhysics : public PhysicsComponent
{
public:
	PlayerPhysics(BaseObject* bo, Bounds* bounds);

	void physicsUpdate(float deltaTime);

	physx::PxCapsuleController* controller;
	physx::PxVec3 tempUp = physx::PxVec3(0.0f, 1.0f, 0.0f);
};

class PlayerRender : public RenderComponent
{
public:
	PlayerRender(BaseObject* bo, Bounds* bounds);

	void preRenderUpdate();
	void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId, shadowPassSkinnedProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;

private:
	void refreshResources();
};

class PlayerCharacter : public BaseObject
{
public:
	PlayerCharacter();
	~PlayerCharacter();

	void streamTokensForLoading(nlohmann::json& object);

	ImGuiComponent* imguiComponent;
	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	Bounds* bounds;
};
