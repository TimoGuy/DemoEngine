#pragma once

#include <PxPhysicsAPI.h>
#include "BaseObject.h"
#include "../render_engine/model/animation/Animator.h"
#include "../render_engine/camera/Camera.h"


typedef unsigned int GLuint;

//
// TODO: place this class inside of somewhere!!!
//
class RopeSimulation
{
public:
	void initializePoints(const std::vector<glm::vec3>& points);
	void setPointPosition(size_t index, float trickleRate, const glm::vec3& position);

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

	void preRenderUpdate();

#ifdef _DEVELOP
	void imguiPropertyPanel();
	void imguiRender();
#endif

	//
	// OLD PLAYERRENDER
	//
	Model* model;
	Animator animator;
	std::map<std::string, Material*> materials;

	Model* bottleModel;
	std::map<std::string, Material*> bottleModelMaterials;
	glm::mat4 bottleModelMatrix, bottleHandModelMatrix;

	glm::vec3 cameraPosition;
	float cameraSpeedMultiplier = 10.0f;

	float currentMaxCamDistance = 0;
	float maxCamDistanceHoldTime = 1.0f, maxCamDistanceHoldTimer = 0;
	float maxCamDistanceShiftSpeed = 1.0f;
	glm::vec3 playerCamOffset = glm::vec3(0, 3, -30);
	glm::vec3 playerCamOffsetIndoor = glm::vec3(0, 1.5f, -10.5f);
	glm::vec2 lookingInput = glm::vec2(0, 0);					// [0-360) on x axis (degrees), [-1,1] on y axis
	glm::vec2 lookingSensitivity = glm::vec2(0.5f, 0.0025f);	// Sensitivity for how much the amount moves

	// TODO: tune these lol (jk jk)
	// And then after, we can make these all consts!!!!
	float groundAcceleration = 1.4f;
	float groundDecceleration = 10.0f;
	float airAcceleration = 2.5f;
	float groundRunSpeed = 0.75f;						// TODO: may wanna change that variable name eh
	float jumpSpeed[2] = { 1.4f, 1.0f };

	float currentRunSpeed = 0.0f;						// This is the value that gets changed
	float immediateTurningRequiredSpeed = 0.1f;			// The maximum velocity you can have to keep the ability to immediately turn (while grounded)

	float leanLerpTime = 10.0f;
	float leanMultiplier = 0.1f;						// NOTE: this is one over the number of degrees of delta required to get the maximum lean (e.g. 1/90degrees = 0.011f)

	glm::vec2 facingDirection = glm::vec2(0, 1);		// NOTE: this is assumed to always be normalized
	float groundedFacingTurnSpeed = 400.0f;
	float airBourneFacingTurnSpeed = 100.0f;			// Much slower than facingTurnSpeed

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
	bool useIndoorCamera;
	float indoorOverlapCheckRadius = 0.75f;
	float indoorOverlapCheckHeight = 20.0f;		// NOTE: very tall, because now I want false positives for indoors so I can see how to improve this algorithm later. Thus, @Incomplete
	float indoorOverlapCheckOffY = 17.5f;

	//
	// Animation Variables
	//
	int animationState = 0;		// 0:Standing	1:Jumping	2:Landing
	int prevAnimState = -1;

	float targetCharacterLeanValue = 0.0f;
	float characterLeanValue = 0.0f;		// [-1, 1], where 0 is no lean
	bool isMoving = false;
	bool waitUntilAnimationFinished = false;

	bool lockFacingDirection = false;
	bool lockJumping = false;

	bool triggerDrinkWaterAnimation = false;

public:		// TODO: make this private (delete this!!!!!!)
	//
	// Rope Simulations
	//
	RopeSimulation leftSideburn, rightSideburn, backAttachment;
};
