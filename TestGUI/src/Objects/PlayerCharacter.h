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

class PlayerPhysics : public PhysicsComponent
{
public:
	PlayerPhysics(BaseObject* bo, Bounds* bounds);

	void physicsUpdate();
	void propagateNewTransform(const glm::mat4& newTransform);
	physx::PxTransform getGlobalPose();

	physx::PxVec3 velocity;

	physx::PxCapsuleController* controller;
	physx::PxVec3 tempUp = physx::PxVec3(0.0f, 1.0f, 0.0f);

	bool getIsGrounded() { return isGrounded; }

	// NOTE: this should not be accessed while !isGrounded, bc it isn't updated unless if on ground <45degrees
	glm::vec3 getGroundedNormal() { return groundedNormal; }

private:
	bool isGrounded;
	glm::vec3 groundedNormal;
};

class PlayerRender : public RenderComponent
{
public:
	PlayerRender(BaseObject* bo, Bounds* bounds);
	~PlayerRender();

	void preRenderUpdate();
	void render(unsigned int irradianceMap, unsigned int prefilterMap, unsigned int brdfLUTTexture);
	void renderShadow(GLuint programId);

	unsigned int pbrShaderProgramId;

	Model model;
	Animator animator;
	GLuint pbrAlbedoTexture, pbrNormalTexture, pbrMetalnessTexture, pbrRoughnessTexture;

	glm::vec3 playerCamOffset = glm::vec3(0, 0, -20);
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

	glm::vec2 facingDirection = glm::vec2(0, 1);		// NOTE: this is assumed to always be normalized
	float facingTurnSpeed = 575.0f;

private:
	void refreshResources();

	glm::vec3 processGroundedMovement(glm::vec2 movementVector);
	glm::vec3 processAirMovement(glm::vec2 movementVector);

	VirtualCamera playerCamera;
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
