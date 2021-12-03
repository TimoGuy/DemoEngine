#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../RenderEngine/RenderEngine.model/RenderEngine.model.animation/Animator.h"
#include "../RenderEngine/RenderEngine.camera/Camera.h"


typedef unsigned int GLuint;

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

class PlayerRender : public RenderComponent
{
public:
	PlayerRender(BaseObject* bo, RenderAABB* bounds);
	~PlayerRender();

	void preRenderUpdate();
	void render();
	void renderShadow(GLuint programId);

	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;

	Model* bottleModel;
	std::map<std::string, Material*> bottleModelMaterials;
	glm::mat4 bottleModelMatrix, bottleHandModelMatrix, finalBottleTransformMatrix;

	float currentMaxCamDistance = 0;
	float maxCamDistanceHoldTime = 2.0f, maxCamDistanceHoldTimer = 0;
	float maxCamDistanceShiftSpeed = 2.0f;
	glm::vec3 playerCamOffset = glm::vec3(0, 3, -30);
	glm::vec2 lookingInput = glm::vec2(0, 0);					// [0-360) on x axis (degrees), [-1,1] on y axis
	glm::vec2 lookingSensitivity = glm::vec2(0.5f, 0.0025f);	// Sensitivity for how much the amount moves

	// TODO: tune these lol (jk jk)
	// And then after, we can make these all consts!!!!
	float groundAcceleration[2] = { 1.4f, 0.5f };
	float groundDecceleration[2] = { 10.0f, 0.25f };
	float airAcceleration[2] = { 2.5f, 2.5f };
	float groundRunSpeed[2] = { 0.75f, 0.65f };						// TODO: may wanna change that variable name eh
	float jumpSpeed[2] = { 1.4f, 1.2f };

	float currentRunSpeed = 0.0f;						// This is the value that gets changed
	float immediateTurningRequiredSpeed = 0.1f;			// The maximum velocity you can have to keep the ability to immediately turn (while grounded)

	float leanLerpTime[2] = { 10.0f, 5.0f };
	float leanMultiplier[2] = { 0.1f, 0.03f };							// NOTE: this is one over the number of degrees of delta required to get the maximum lean (e.g. 1/90degrees = 0.011f)

	glm::vec2 facingDirection = glm::vec2(0, 1);		// NOTE: this is assumed to always be normalized
	float facingTurnSpeed[2] = { 400.0f, 50.0f };
	float airBourneFacingTurnSpeed[2] = { 100.0f, 50.0f };			// Much slower than facingTurnSpeed

	float modelOffsetY = -3.35f;
	float animationSpeed = 42.0f;

private:
	void refreshResources();

	void processMovement();
	physx::PxVec3 processGroundedMovement(const glm::vec2& movementVector);
	physx::PxVec3 processAirMovement(const glm::vec2& movementVector);

	void processActions();

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

	bool triggerDrinkWaterAnimation = false;

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

	PhysicsComponent* physicsComponent;
	RenderComponent* renderComponent;

	LightComponent* getLightComponent() { return nullptr; }
	PhysicsComponent* getPhysicsComponent() { return physicsComponent; }
	RenderComponent* getRenderComponent() { return renderComponent; }

	RenderAABB* bounds;

#ifdef _DEBUG
	void propertyPanelImGui();
	void renderImGui();
#endif
};
