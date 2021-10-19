#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"


struct VirtualCamera;

class PlayerImGui : public ImGuiComponent
{
public:
	PlayerImGui(BaseObject* bo, Bounds* bounds) : ImGuiComponent(bo, bounds, "Player Controller") {}

	void propertyPanelImGui();
	void renderImGui();
};

class PlayerRender : public RenderComponent
{
public:
	PlayerRender(BaseObject* bo, Bounds* bounds);
	~PlayerRender();

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId;

	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;

	glm::vec3 playerCamOffset = glm::vec3(0, 3, -30);
	glm::vec2 lookingInput = glm::vec2(0, 0);					// [0-360) on x axis (degrees), [-1,1] on y axis
	glm::vec2 lookingSensitivity = glm::vec2(0.5f, 0.0025f);	// Sensitivity for how much the amount moves

	// TODO: tune these lol (jk jk)
	// And then after, we can make these all consts!!!!
	float jumpSpeed = 2.5f;
	float groundAcceleration = 4.2f;
	float groundDecceleration = 9.0f;
	float airAcceleration = 2.5f;
	float groundRunSpeed = 1.0f;						// TODO: may wanna change that variable name eh
	float currentRunSpeed = 0.0f;						// This is the value that gets changed
	float immediateTurningRequiredSpeed = 0.1f;			// The maximum velocity you can have to keep the ability to immediately turn (while grounded)

	glm::vec2 facingDirection = glm::vec2(0, 1);		// NOTE: this is assumed to always be normalized
	float facingTurnSpeed = 575.0f;
	float airBourneFacingTurnSpeed = 100.0f;			// Much slower than facingTurnSpeed

	float leanLerpTime = 10.0f;
	float modelOffsetY = -3.35f;

private:
	void refreshResources();

	void processMovement();
	physx::PxVec3 processGroundedMovement(const glm::vec2& movementVector);
	physx::PxVec3 processAirMovement(const glm::vec2& movementVector);

	void processAnimation();

	VirtualCamera playerCamera;

	float targetCharacterLeanValue = 0.0f;
	float characterLeanValue = 0.0f;		// [-1, 1], where 0 is no lean
	bool isMoving = false;
};

class PlayerCharacter : public BaseObject
{
public:
	static const std::string TYPE_NAME;

	PlayerCharacter();
	~PlayerCharacter();

	void loadPropertiesFromJson(nlohmann::json& object);
	nlohmann::json savePropertiesToJson();

	ImGuiComponent* imguiComponent;
	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	ImGuiComponent* getImguiComponent() { return imguiComponent; }
	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	Bounds* bounds;
};
