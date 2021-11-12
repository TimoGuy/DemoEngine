#pragma once

#include <glad/glad.h>
#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"

//
// TODO: place this class inside of somewhere!!!
//
class RopeSimulation
{
public:
	void initializePoints(const std::vector<glm::vec3>& points);
	void setPointPosition(size_t index, const glm::vec3& position);

	void simulateRope(float gravityMultiplier);

	glm::vec3 getPoint(size_t index) { return points[index]; }

	bool isFirstTime = true;
	bool limitTo45degrees = false;

private:
	std::vector<glm::vec3> points;
	std::vector<glm::vec3> prevPoints;
	std::vector<float> distances;
};


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

	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;

	Model* bottleModel;
	std::map<std::string, Material*> bottleModelMaterials;
	glm::mat4 bottleModelMatrix, bottleHandModelMatrix;

	glm::vec3 playerCamOffset = glm::vec3(0, 3, -30);
	glm::vec2 lookingInput = glm::vec2(0, 0);					// [0-360) on x axis (degrees), [-1,1] on y axis
	glm::vec2 lookingSensitivity = glm::vec2(0.5f, 0.0025f);	// Sensitivity for how much the amount moves

	// TODO: tune these lol (jk jk)
	// And then after, we can make these all consts!!!!
	float jumpSpeed = 2.0f;								// Previously was 2.5f, however, after looking at it with a different camera angle, now we know that's way too much
	float groundAcceleration = 4.2f;
	float groundDecceleration = 9.0f;
	float airAcceleration = 2.5f;
	float groundRunSpeed = 1.0f;						// TODO: may wanna change that variable name eh
	float currentRunSpeed = 0.0f;						// This is the value that gets changed
	float immediateTurningRequiredSpeed = 0.1f;			// The maximum velocity you can have to keep the ability to immediately turn (while grounded)

	glm::vec2 facingDirection = glm::vec2(0, 1);		// NOTE: this is assumed to always be normalized
	float facingTurnSpeed = 400.0f;
	float airBourneFacingTurnSpeed = 100.0f;			// Much slower than facingTurnSpeed

	float leanLerpTime = 10.0f;
	float leanMultiplier = 0.1f;							// NOTE: this is one over the number of degrees of delta required to get the maximum lean (e.g. 1/90degrees = 0.011f)
	float modelOffsetY = -3.35f;

	float animationSpeed = 42.0f;

private:
	void refreshResources();

	void processMovement();
	physx::PxVec3 processGroundedMovement(const glm::vec2& movementVector);
	physx::PxVec3 processAirMovement(const glm::vec2& movementVector);

	void processAnimation();

	VirtualCamera playerCamera;

	//
	// Animation Variables
	//
	int animationState = 0;		// 0:Standing	1:Jumping	2:Landing
	int prevAnimState = -1;

	float targetCharacterLeanValue = 0.0f;
	float characterLeanValue = 0.0f;		// [-1, 1], where 0 is no lean
	bool isMoving = false;
	bool waitUntilAnimationFinished = false;
	bool prevIsGrounded;

	bool lockFacingDirection = false;
	bool lockJumping = false;

public:		// TODO: make this private (delete this!!!!!!)
	//
	// Rope Simulations
	//
	RopeSimulation leftSideburn, rightSideburn, backAttachment;
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
